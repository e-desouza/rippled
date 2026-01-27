//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2024 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <xrpld/app/ledger/Ledger.h>
#include <xrpld/app/overlay/handlers/LedgerRequestHandler.h>
#include <xrpld/overlay/Message.h>
#include <xrpld/overlay/detail/Tuning.h>

#include <xrpl/basics/Blob.h>
#include <xrpl/protocol/Serializer.h>
#include <xrpl/shamap/SHAMap.h>
#include <xrpl/shamap/SHAMapNodeID.h>

namespace xrpl {

std::shared_ptr<Ledger const>
LedgerRequestHandler::getLedger(std::shared_ptr<protocol::TMGetLedger> const& m)
{
    JLOG(journal_.trace()) << "getLedger: Ledger";

    std::shared_ptr<Ledger const> ledger;

    if (m->has_ledgerhash())
    {
        // Attempt to find ledger by hash
        uint256 const ledgerHash{m->ledgerhash()};
        ledger = ledgerMaster_.getLedgerByHash(ledgerHash);
        if (!ledger)
        {
            JLOG(journal_.trace())
                << "getLedger: Don't have ledger with hash " << ledgerHash;
        }
    }
    else if (m->has_ledgerseq())
    {
        // Attempt to find ledger by sequence
        if (m->ledgerseq() < ledgerMaster_.getEarliestFetch())
        {
            JLOG(journal_.debug())
                << "getLedger: Early ledger sequence request";
        }
        else
        {
            ledger = ledgerMaster_.getLedgerBySeq(m->ledgerseq());
            if (!ledger)
            {
                JLOG(journal_.debug())
                    << "getLedger: Don't have ledger with sequence "
                    << m->ledgerseq();
            }
        }
    }
    else if (m->has_ltype() && m->ltype() == protocol::ltCLOSED)
    {
        ledger = ledgerMaster_.getClosedLedger();
    }

    if (ledger)
    {
        // Validate retrieved ledger sequence
        auto const ledgerSeq{ledger->header().seq};
        if (m->has_ledgerseq())
        {
            if (ledgerSeq != m->ledgerseq())
            {
                JLOG(journal_.warn())
                    << "getLedger: Ledger sequence mismatch: requested "
                    << m->ledgerseq() << ", got " << ledgerSeq;
                return nullptr;
            }
        }
    }

    return ledger;
}

void
LedgerRequestHandler::sendLedgerBase(
    std::shared_ptr<Ledger const> const& ledger,
    protocol::TMLedgerData& ledgerData)
{
    JLOG(journal_.trace()) << "sendLedgerBase: Base data";

    Serializer s(sizeof(LedgerHeader));
    addRaw(ledger->header(), s);
    ledgerData.add_nodes()->set_nodedata(s.getDataPtr(), s.getLength());

    auto const& stateMap{ledger->stateMap()};
    if (stateMap.getHash() != beast::zero)
    {
        // Return account state root node if possible
        Serializer root(768);

        stateMap.serializeRoot(root);
        ledgerData.add_nodes()->set_nodedata(
            root.getDataPtr(), root.getLength());

        if (ledger->header().txHash != beast::zero)
        {
            auto const& txMap{ledger->txMap()};
            if (txMap.getHash() != beast::zero)
            {
                // Return TX root node if possible
                root.erase();
                txMap.serializeRoot(root);
                ledgerData.add_nodes()->set_nodedata(
                    root.getDataPtr(), root.getLength());
            }
        }
    }
}

std::shared_ptr<Message>
LedgerRequestHandler::processLedgerDataRequest(
    std::shared_ptr<protocol::TMGetLedger> const& m,
    bool isHighLatency)
{
    auto const itype{m->itype()};

    // This handler only processes ledger-based requests
    if (itype == protocol::liTS_CANDIDATE)
    {
        JLOG(journal_.error())
            << "processLedgerDataRequest: liTS_CANDIDATE not supported";
        return nullptr;
    }

    auto ledger = getLedger(m);
    if (!ledger)
        return nullptr;

    protocol::TMLedgerData ledgerData;

    // Fill out the reply
    auto const ledgerHash{ledger->header().hash};
    ledgerData.set_ledgerhash(ledgerHash.begin(), ledgerHash.size());
    ledgerData.set_ledgerseq(ledger->header().seq);
    ledgerData.set_type(itype);
    if (m->has_requestcookie())
        ledgerData.set_requestcookie(m->requestcookie());

    SHAMap const* map{nullptr};

    switch (itype)
    {
        case protocol::liBASE:
            sendLedgerBase(ledger, ledgerData);
            return std::make_shared<Message>(
                ledgerData, protocol::mtLEDGER_DATA);

        case protocol::liTX_NODE:
            map = &ledger->txMap();
            JLOG(journal_.trace()) << "processLedgerDataRequest: TX map hash "
                                   << to_string(map->getHash());
            break;

        case protocol::liAS_NODE:
            map = &ledger->stateMap();
            JLOG(journal_.trace())
                << "processLedgerDataRequest: Account state map hash "
                << to_string(map->getHash());
            break;

        default:
            JLOG(journal_.error())
                << "processLedgerDataRequest: Invalid ledger info type";
            return nullptr;
    }

    if (!map)
    {
        JLOG(journal_.warn()) << "processLedgerDataRequest: Unable to find map";
        return nullptr;
    }

    // Add requested node data to reply
    if (m->nodeids_size() > 0)
    {
        auto const queryDepth{
            m->has_querydepth() ? m->querydepth() : (isHighLatency ? 2 : 1)};

        std::vector<std::pair<SHAMapNodeID, Blob>> data;

        for (int i = 0; i < m->nodeids_size() &&
             ledgerData.nodes_size() < Tuning::softMaxReplyNodes;
             ++i)
        {
            auto const shaMapNodeId{deserializeSHAMapNodeID(m->nodeids(i))};

            data.clear();
            data.reserve(Tuning::softMaxReplyNodes);

            try
            {
                bool const fatLeaves = true;
                if (map->getNodeFat(*shaMapNodeId, data, fatLeaves, queryDepth))
                {
                    JLOG(journal_.trace())
                        << "processLedgerDataRequest: getNodeFat got "
                        << data.size() << " nodes";

                    for (auto const& d : data)
                    {
                        if (ledgerData.nodes_size() >=
                            Tuning::hardMaxReplyNodes)
                            break;
                        protocol::TMLedgerNode* node{ledgerData.add_nodes()};
                        node->set_nodeid(d.first.getRawString());
                        node->set_nodedata(d.second.data(), d.second.size());
                    }
                }
                else
                {
                    JLOG(journal_.warn())
                        << "processLedgerDataRequest: getNodeFat returns false";
                }
            }
            catch (std::exception const& e)
            {
                std::string info;
                switch (itype)
                {
                    case protocol::liBASE:
                        info = "Ledger base";
                        break;
                    case protocol::liTX_NODE:
                        info = "TX node";
                        break;
                    case protocol::liAS_NODE:
                        info = "AS node";
                        break;
                    default:
                        info = "Invalid";
                        break;
                }

                if (!m->has_ledgerhash())
                    info += ", no hash specified";

                JLOG(journal_.warn())
                    << "processLedgerDataRequest: getNodeFat with nodeId "
                    << *shaMapNodeId << " and ledger info type " << info
                    << " throws exception: " << e.what();
            }
        }

        JLOG(journal_.info())
            << "processLedgerDataRequest: Got request for " << m->nodeids_size()
            << " nodes at depth " << queryDepth << ", return "
            << ledgerData.nodes_size() << " nodes";
    }

    if (ledgerData.nodes_size() == 0)
        return nullptr;

    return std::make_shared<Message>(ledgerData, protocol::mtLEDGER_DATA);
}

}  // namespace xrpl
