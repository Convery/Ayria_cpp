/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-01-22
    License: MIT
*/

#include <Stdinclude.hpp>
#include "Global.hpp"

namespace Clientinfo
{
    std::vector<Client_t> Remoteclients;

    // The only interface other code should use.
    std::vector<Client_t *> getRemoteclients()
    {
        std::vector<Client_t *> Result; Result.reserve(Remoteclients.size());
        for (auto it = Remoteclients.begin(); it != Remoteclients.end(); ++it)
            Result.push_back(&*it);
        return Result;
    }

    // Fetch delta info about clients.
    static uint32_t Lastupdate = 0;
    void Updateremoteclients()
    {
        // TODO(tcn): Fetch data from some server, delta since timestamp.
    }
}
