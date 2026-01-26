#include <xrpld/overlay/detail/handlers/StatusChangeMessageHandler.h>

#include <xrpld/app/main/Application.h>
#include <xrpld/app/misc/NetworkOPs.h>
#include <xrpld/overlay/detail/PeerImp.h>

#include <xrpl/json/json_value.h>
#include <xrpl/protocol/jss.h>

#include <xrpl/protocol/messages.h>

namespace xrpl {

void
StatusChangeMessageHandler::publishPeerStatus(
    std::shared_ptr<protocol::TMStatusChange> const& m,
    PeerImp& peer,
    uint256 const& closedLedgerHash)
{
    peer.app_.getOPs().pubPeerStatus([=]() -> Json::Value {
        Json::Value j = Json::objectValue;

        if (m->has_newstatus())
        {
            switch (m->newstatus())
            {
                case protocol::nsCONNECTING:
                    j[jss::status] = "CONNECTING";
                    break;
                case protocol::nsCONNECTED:
                    j[jss::status] = "CONNECTED";
                    break;
                case protocol::nsMONITORING:
                    j[jss::status] = "MONITORING";
                    break;
                case protocol::nsVALIDATING:
                    j[jss::status] = "VALIDATING";
                    break;
                case protocol::nsSHUTTING:
                    j[jss::status] = "SHUTTING";
                    break;
            }
        }

        if (m->has_newevent())
        {
            switch (m->newevent())
            {
                case protocol::neCLOSING_LEDGER:
                    j[jss::action] = "CLOSING_LEDGER";
                    break;
                case protocol::neACCEPTED_LEDGER:
                    j[jss::action] = "ACCEPTED_LEDGER";
                    break;
                case protocol::neSWITCHED_LEDGER:
                    j[jss::action] = "SWITCHED_LEDGER";
                    break;
                case protocol::neLOST_SYNC:
                    j[jss::action] = "LOST_SYNC";
                    break;
            }
        }

        if (m->has_ledgerseq())
        {
            j[jss::ledger_index] = m->ledgerseq();
        }

        if (m->has_ledgerhash())
        {
            j[jss::ledger_hash] = to_string(closedLedgerHash);
        }

        if (m->has_networktime())
        {
            j[jss::date] = Json::UInt(m->networktime());
        }

        if (m->has_firstseq() && m->has_lastseq())
        {
            j[jss::ledger_index_min] = Json::UInt(m->firstseq());
            j[jss::ledger_index_max] = Json::UInt(m->lastseq());
        }

        return j;
    });
}

}  // namespace xrpl

