
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "react/Signal.h"
#include "react/EventStream.h"
#include "react/Operations.h"
#include "react/ReactiveObject.h"

//#include "react/propagation/FloodingEngine.h"
//#include "react/propagation/SourceSetEngine.h"
//#include "react/propagation/TopoSortEngine.h"
//#include "react/propagation/ELMEngine.h"

using namespace std;
using namespace react;

// Defines a domain.
// Each domain represents a separate dependency graph, managed by a dedicated propagation engine.
// Reactives of different domains can not be combined.
REACTIVE_DOMAIN(D);

void SignalExample1()
{
    cout << "Signal Example 1" << endl;

    auto width = D::MakeVar(60);
    auto height = D::MakeVar(70);
    auto depth = D::MakeVar(8);

    auto area    = width * height;
    auto volume    = area * depth;

    cout << "t0" << endl;
    cout << "\tArea: " << area() << endl;
    cout << "\tVolume: " << volume() << endl;

    width <<= 90;
    depth <<= 80;

    cout << "t1" << endl;
    cout << "\tArea: " << area() << endl;
    cout << "\tVolume: " << volume() << endl;

    cout << endl;
}

void SignalExample2()
{
    cout << "Signal Example 2" << endl;

    auto width = D::MakeVar(60);
    auto height = D::MakeVar(70);
    auto depth = D::MakeVar(8);

    auto volume = (width,height,depth) ->* [] (int w, int h, int d) {
        return w * h * d;
    };

    // Observe returns an observer handle, which can be used to detach the observer explicitly.
    // This observer handle holds a shared_ptr to the subject, so as long as it exists,
    // the subject will not be destroyed.
    // The lifetime of the observer itself is tied to the subject.
    Observe(volume, [] (int v) {
        cout << "Volume changed to: " << v << endl;
    });

    D::DoTransaction([&] {
        width <<= 90;
        depth <<= 80;
    });

    cout << endl;
}

void SignalExample3()
{
    cout << "Signal Example 3" << endl;

    auto src = D::MakeVar(0);

    // Input values can be manipulated imperatively in observers.
    // Inputs are implicitly thread-safe, buffered and executed in a continuation turn.
    // This continuation turn is queued just like a regular turn.
    // If other turns are already queued, they are executed before the continuation.
    Observe(src, [&] (int v) {
        cout << "V: " << v << endl;
        if (v < 10)
            src <<= v+1;
    });

    src <<= 1;

    cout << endl;
}

void EventExample1()
{
    cout << "Event Example 1" << endl;

    auto numbers1 = D::MakeEventSource<int>();
    auto numbers2 = D::MakeEventSource<int>();

    auto anyNumber = numbers1 | numbers2;

    Observe(anyNumber, [] (int v) {
        cout << "Number: " << v << endl;
    });

    numbers1 << 10 << 20 << 30;
    numbers2 << 40 << 50 << 60;

    cout << endl;
}

void EventExample2()
{
    cout << "Event Example 2" << endl;

    // The event type can be omitted if not required, in which case the event
    // stream just indicates that it has fired, i.e. it behaves like a token stream.
    auto emitter = D::MakeEventSource();

    auto counter = Iterate(0, emitter, Incrementer<int>());

    // In this case, the observer func must not declare a parameter for token streams.
    Observe(emitter, [] {
        cout << "Emitter fired!" << endl;
    });

    // Using .Emit() to fire rather than "<< value"
    for (int i=0; i<5; i++)
        emitter.Emit();

    cout << "Counted " << counter() << " events" << endl;
    cout << endl;
}

// ReactiveObject
class Person : public ReactiveObject<D>
{
public:
    VarSignal<int>  Age     = MakeVar(1);
    Signal<int>     Health  = 100 - Age;
    Signal<int>     Wisdom  = Age * Age / 100;

    // Note ICC compiler bug:
    // Initializing them directly uses the same lambda for both signals
    Observer    wisdomObs;
    Observer    weaknessObs;

    Person()
    {
        wisdomObs = Observe(Wisdom > 50, [] (bool isWise)
        {
            if (isWise) cout << "I'll do it next week!" << endl;
            else        cout << "I'll do it next month!" << endl;
        });

        weaknessObs = Observe(Health < 25, [] (bool isWeak)
        {
            if (isWeak) cout << ":<" << endl;
            else        cout << ":D" << endl;
        });
    }

    bool operator==(const Person& other) const
    {
        return this == &other;
    }
};

void ObjectExample1()
{
    cout << "Object Example 1" << endl;

    Person somePerson{ "Peter" };

    somePerson.Age <<= 30;
    somePerson.Age <<= 60;
    somePerson.Age <<= 90;

    cout << "Health: " << somePerson.Health() << endl;
    cout << "Wisdom: " << somePerson.Wisdom() << endl;

    cout << endl;
}

class Company : public ReactiveObject<D>
{
public:
    VarSignal<string>    Name;

    Company(const char* name) :
        Name{ MakeVar(string(name)) }
    {
    }

    inline bool operator==(const Company& other) const
    {
        return this == &other;
    }
};

class Manager : public ReactiveObject<D>
{
    Observer nameObs;

public:
    VarRefSignal<Company>    CurrentCompany;

    Manager(Company& c) :
        CurrentCompany{ MakeVar(std::ref(c)) }
    {
        nameObs = REACTIVE_REF(CurrentCompany, Name).Observe([] (string name) {
            cout << "Manager: Now managing " << name.c_str() << endl;
        });
    }
};

void ObjectExample2()
{
    cout << "Object Example 2" << endl;

    Company company1{ "Cellnet" };
    Company company2{ "Borland" };

    Manager manager{ company1 };

    company1.Name <<= string("BT Cellnet");
    company2.Name <<= string("Inprise");

   manager.CurrentCompany <<= std::ref(company2);

    company1.Name <<= string("O2");
    company2.Name <<= string("Borland");

    cout << endl;
}

void FoldExample1()
{
    cout << "Fold Example 1" << endl;

    auto src = D::MakeEventSource<int>();
    auto fold1 = Fold(0, src, [] (int v, int d) {
        return v + d;
    });

    for (auto i=1; i<=100; i++)
        src << i;

    cout << fold1() << endl;

    auto charSrc = D::MakeEventSource<char>();
    auto strFold = Fold(std::string(""), charSrc, [] (std::string s, char c) {
        return s + c;
    });

    charSrc << 'T' << 'e' << 's' << 't';

    cout << "Str: " << strFold() << endl;
}

#include "tbb/tick_count.h"

void Debug()
{
    cout << "A" << endl;
    {
        int x = 0;

        auto t0 = tbb::tick_count::now();

        for (int i=0; i<10000000; i++)
        {
            x += i;
        }

        auto t1 = tbb::tick_count::now();

        auto d = (t1 - t0).seconds();

        cout << x << endl;
        cout << d << endl;
    }

    cout << "B" << endl;
    {
        auto x = D::MakeVar(0);

        auto t0 = tbb::tick_count::now();

        for (int i=0; i<10000000; i++)
        {
            x <<= i;
        }

        auto t1 = tbb::tick_count::now();

        auto d = (t1 - t0).seconds();

        cout << d << endl;
    }
}

#ifndef REACT_DISABLE_REACTORS
#include "react/Reactor.h"

void LoopTest()
{
    cout << "ReactiveLoop Example 1" << endl;

    using PointT = pair<int,int>;
    using PathT  = vector<PointT>;

    vector<PathT> paths;

    auto mouseDown = D::MakeEventSource<PointT>();
    auto mouseUp   = D::MakeEventSource<PointT>();
    auto mouseMove = D::MakeEventSource<PointT>();

    D::ReactiveLoop loop
    {
        [&] (D::ReactiveLoop::Context& ctx)
        {
            PathT points;

            points.emplace_back(ctx.Take(mouseDown));

            ctx.RepeatUntil(mouseUp, [&] {
                points.emplace_back(ctx.Take(mouseMove));
            });

            points.emplace_back(ctx.Take(mouseUp));

            paths.push_back(points);
        }
    };

    mouseDown << PointT(1,1);
    mouseMove << PointT(2,2) << PointT(3,3) << PointT(4,4);
    mouseUp   << PointT(5,5);

    mouseMove << PointT(999,999);

    mouseDown << PointT(10,10);
    mouseMove << PointT(20,20);
    mouseUp   << PointT(30,30);

    for (const auto& path : paths)
    {
        cout << "Path: ";
        for (const auto& point : path)
            cout << "(" << point.first << "," << point.second << ")   ";
        cout << endl;
    }
}
#endif //REACT_DISABLE_REACTORS

int main()
{
    SignalExample1();
    SignalExample2();
    SignalExample3();

    EventExample1();
    EventExample2();

    ObjectExample1();
    ObjectExample2();

    FoldExample1();

    LoopTest();

    //Debug();

    return 0;
}