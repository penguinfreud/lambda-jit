#ifndef MAIN_HPP_
#define MAIN_HPP_

#include <string>
#include <iostream>

#include <deque>
#include <utility>

namespace jit {

    bool isNum(int c);

    bool isNameChar(int c);

    class Ast;

    class Parser {
    public:
        Parser(std::string &s);

        int peek();

        void advance();

        bool match(int c);

        bool match(const std::string &s);

        bool matchId(const std::string &s, bool advance);

        void skipSpace();

        bool finished();

        void fail(const std::string &msg);

        bool ok();

        friend std::string *parseName(Parser &parser);

    private:
        std::string &str;
        unsigned long length, pos;
        bool _ok;
    };

    class Environ {
    public:
        Environ();

        Environ(Environ &env);

        void put(std::string *key, Ast *ast);

        Ast *get(std::string *key, bool &ok);

        void unput();

        ~Environ();

    private:
        std::deque<std::pair<std::string *, Ast *> *> list;
    };

    class Ast {
    public:
        enum Ast_type {
            NUM, VARIABLE, APPLICATION, LET, LAMBDA
        };

        Ast(Ast_type _type);

        virtual ~Ast();

        virtual Ast *operator()(Environ &env, bool &ok);

        virtual void print(std::ostream &os) const;
    private:
        Ast_type type;
    };

    class Num : public Ast {
    private:
        int num;

    public:
        Num(int x);

        Ast *operator()(Environ &env, bool &ok);

        void print(std::ostream &os) const;
    };

    class Var : public Ast {
    private:
        std::string *name;

    public:
        Var(std::string *s);

        Ast *operator()(Environ &env, bool &ok);

        ~Var();

        void print(std::ostream &os) const;
    };

    class Application : public Ast {
    private:
        Ast *fun, *arg;

    public:
        Application(Ast *f, Ast *a);

        Ast *operator()(Environ &env, bool &ok);

        ~Application();

        void print(std::ostream &os) const;
    };

    class Let : public Ast {
    private:
        std::string *var;
        Ast *value, *expr;

    public:
        Let(std::string *x, Ast *v, Ast *e);

        Ast *operator()(Environ &env, bool &ok);

        ~Let();

        void print(std::ostream &os) const;
    };

    class Lambda : public Ast {
    private:
        std::string *var;
        Ast *expr;
        Environ *env;

    public:
        Lambda(std::string *v, Ast *e);

        Ast *operator()(Environ &env, bool &ok);

        Ast *apply(Environ &env, bool &ok, Ast *ast);

        ~Lambda();

        void print(std::ostream &os) const;
    };

    std::ostream &operator<<(std::ostream &os, const Ast &ast);

    Ast *parseExpr(Parser &parser);
}

#endif /* MAIN_HPP_ */