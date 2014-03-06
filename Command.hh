#ifndef COMMAND_HH
#define COMMAND_HH

// base pointer class
class Command
{
public:
  virtual ~Command()
        {}
  virtual int execute()=0;
};

// e.g. for static functions with 0 args
template<class T>
class Trigger : public Command
{
public:
  Trigger(T f) : _f(f) {}
  int execute() { (*_f)(); return 0;}
private:
  T _f;
};
template<class T>
Command* makeTrigger( T f)
{return new Trigger<T>(f);}

// for bound member functions with 0 args
template<class T,typename RV>
class Trigger_0 : public Command
{
public:
  Trigger_0( T& t, RV (T::*f)()) : _t(t),_f(f) {}
  int execute() { (_t.*_f)(); return 0;}
private:
  T& _t;
  RV (T::*_f)();
};
template<class T,typename RV>
Command* makeTrigger_0( T& l, RV (T::*f)())
{return new Trigger_0<T,RV>( l, f);}

// for bound member functions with 1 arg
template<class T,typename RV,typename ARG>
class Trigger_1 : public Command
{
public:
  Trigger_1( T& t, RV (T::*f)(ARG), ARG a) : _t(t),_f(f),_a(a) {}
  int execute() { (_t.*_f)(_a); return 0;}
private:
  T& _t;
  RV (T::*_f)(ARG);
  ARG _a;
};
template<class T,typename RV,typename ARG>
Command* makeTrigger_1( T& l, RV (T::*f)(ARG), ARG a)
{return new Trigger_1<T,RV,ARG>( l, f, a);}

// for bound member functions with 2 args
template<class T,typename RV,typename ARG1,typename ARG2>
class Trigger_2 : public Command
{
public:
  Trigger_2( T& t, RV (T::*f)(ARG1,ARG2), ARG1 a1, ARG2 a2)
          : _t(t),_f(f),_a1(a1),_a2(a2) {}
  int execute() { (_t.*_f)(_a1,_a2); return 0;}
private:
  T& _t;
  RV (T::*_f)(ARG1,ARG2);
  ARG1 _a1;
  ARG2 _a2;
};
template<class T,typename RV,typename ARG1,typename ARG2>
Command* makeTrigger_2( T& l, RV (T::*f)(ARG1,ARG2), ARG1 a1, ARG2 a2)
{return new Trigger_2<T,RV,ARG1,ARG2>( l, f, a1, a2);}

#endif
