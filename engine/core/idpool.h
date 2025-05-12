#pragma once
//------------------------------------------------------------------------------
/**
    @file idpool.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <vector>
#include <queue>

namespace Util
{
    //------------------------------------------------------------------------------
    /**
        Generation pool
        Assumes 22 bits index and 10 bits generation
    */
    template<typename ID_T>
    class IdPool
    {
    public:
        // default constructor
        IdPool();

        /// allocate a new id, returns false if the id was reused
        bool Allocate(ID_T& i);
        /// remove an id
        void Deallocate(ID_T i);
        /// check if valid
        bool IsValid(ID_T i) const;

        /// array containing generation value for every index
        std::vector<uint16_t> generations;
        /// stores freed indices
        std::queue<uint32_t> freeIds;
    };

    //------------------------------------------------------------------------------
    /**
    */
    template<typename ID_T>
    IdPool<ID_T>::IdPool()
    {
        this->generations.reserve(1024);
        // TODO: No reserve in queue? que?
        //this->freeIds.reserve(2048);
    }

    //------------------------------------------------------------------------------
    /**
    */
    template<typename ID_T>
    bool
        IdPool<ID_T>::Allocate(ID_T& i)
    {
        // make sure we don't run out of generations too fast
        // by always deallocating at least n ids before recycling
        if (this->freeIds.size() < 1024)
        {
            this->generations.push_back(0);
            i.index = this->generations.size() - 1;
            n_assert2(i.index < 0x003FFFFF, "index overflow");
            i.generation = 0;
            return true;
        }
        else
        {
            uint32_t const index = this->freeIds.front();
            this->freeIds.pop();
            i.index = index;
            i.generation = this->generations[index];
            return false;
        }
    }

    //------------------------------------------------------------------------------
    /**
    */
    template<typename ID_T>
    void
        IdPool<ID_T>::Deallocate(ID_T i)
    {
        n_assert2(this->IsValid(i), "Tried to deallocate invalid/destroyed id!");
        this->freeIds.push(i.index);
#if _DEBUG
        // if you get this warning, you might want to consider reserving more bits for the generation.
        if (this->generations[i.index] <= 0x3FF) printf("WARNING: Id generation overflow!");
#endif
        this->generations[i.index]++;

    }

    //------------------------------------------------------------------------------
    /**
    */
    template<typename ID_T>
    bool
        IdPool<ID_T>::IsValid(ID_T i) const
    {
        return i.index < (uint32_t)this->generations.size() &&
            i.generation == this->generations[i.index];
    }

}
