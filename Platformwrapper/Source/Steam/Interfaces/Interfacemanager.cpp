/*
    Initial author: Convery (tcn@ayria.se)
    Started: 09-04-2019
    License: MIT
*/

#include "../Steam.hpp"

namespace Steam
{
    std::vector<std::pair<Interfacetype_t, Interface_t *>> *Interfacestore;
    phmap::flat_hash_map<Interfacetype_t, Interface_t *> Currentinterfaces;
    phmap::flat_hash_map<std::string_view, Interface_t *> *Interfacenames;

    // A nice little dummy interface for debugging.
    struct Dummyinterface : Interface_t
    {
        template<int N> void Dummyfunc() { Errorprint(__FUNCSIG__); assert(false); };
        Dummyinterface()
        {
            std::any K;

            #define Createmethod(N) K = &Dummyinterface::Dummyfunc<N>; VTABLE[N] = *(void **)&K;

            // 70 methods here.
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
            Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__); Createmethod(__COUNTER__);
        }
        #undef Createmethod
    };

    // Return a specific version of the interface by name or the latest by their category / type.
    void Registerinterface(Interfacetype_t Type, std::string_view Name, Interface_t *Interface)
    {
        if (!Interfacestore) Interfacestore = new std::remove_pointer_t<decltype(Interfacestore)>;
        if (!Interfacenames) Interfacenames = new std::remove_pointer_t<decltype(Interfacenames)>;

        Interfacestore->push_back(std::make_pair(Type, Interface));
        Interfacenames->emplace(Name, Interface);
    }
    Interface_t **Fetchinterface(std::string_view Name)
    {
        // See if we even have the interface implemented.
        const auto Result = Interfacenames->find(Name);
        if (Result == Interfacenames->end())
        {
            // Return the dummy interface for debugging.
            Errorprint(va("Interface missing for interface-name %*s", Name.size(), Name.data()));
            static auto Debug = new Dummyinterface();
            return (Interface_t * *)&Debug;
        }

        // Find the type from the store.
        for (const auto &[Type, Ptr] : *Interfacestore)
        {
            if (Ptr == Result->second)
            {
                Currentinterfaces.emplace(Type, Ptr);
                return &Currentinterfaces[Type];
            }
        }

        // This should never be hit.
        assert(false); return nullptr;
    }
    Interface_t **Fetchinterface(Interfacetype_t Type)
    {
        // See if we have any interface selected for this type.
        if (const auto Result = Currentinterfaces.find(Type); Result != Currentinterfaces.end())
            return &Result->second;

        // Return the dummy interface for debugging.
        Errorprint(va("Interface missing for interface-type %i", Type));
        static auto Debug = new Dummyinterface();
        return (Interface_t **)&Debug;
    }
}
