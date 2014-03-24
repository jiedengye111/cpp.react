
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <memory>
#include <map>
#include <cstdint>
#include <functional>
#include <iostream>
#include <queue>
#include <chrono>

#include "react/common/Types.h"
#include "react/logging/Logging.h"

#include "tbb/concurrent_vector.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventLog
///////////////////////////////////////////////////////////////////////////////////////////////////
class EventLog
{
    typedef std::chrono::time_point<std::chrono::high_resolution_clock> Timestamp;

    class Entry
    {
    public:
        Entry();
        Entry(const Entry& other);

        explicit Entry(IEventRecord* ptr);

        Entry& operator=(Entry& rhs);

        inline const char* EventId() const
        {
            return data_->EventId();
        }

        inline const Timestamp& Time() const
        {
            return time_;
        }

        inline bool operator<(const Entry& other) const
        {
            return time_ < other.time_;
        }

        inline void Release()
        {
            delete data_;
        }

        void Serialize(std::ostream& out, const Timestamp& startTime) const;
        bool Equals(const Entry& other) const;

    private:
        Timestamp        time_;
        IEventRecord*    data_;
    };

public:

    EventLog();
    ~EventLog();

    void    Print();
    void    Write(std::ostream& out);
    void    Clear();

    template
    <
        typename TEventRecord,
        typename ... TArgs
    >
    void Append(TArgs ... args)
    {
        entries_.push_back(Entry(new TEventRecord(args ...)));
    }

private:
    typedef tbb::concurrent_vector<Entry>    LogEntries;

    LogEntries        entries_;
    Timestamp        startTime_;
};

/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

using EventLog = REACT_IMPL::EventLog;

/******************************************/ REACT_END /******************************************/
