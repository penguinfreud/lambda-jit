#include "main.h"

namespace jit {
    bool isNum(int c) {
        return c >= '0' && c <= '9';
    }

    bool isNameChar(int c) {
        return c >= 'A' && c <= 'Z' ||
               c >= 'a' && c <= 'z';
    }

    Parser::Parser(std::string &s) : str(s), length(s.size()), pos(0), _ok(true) { }

    int Parser::peek() {
        return pos < length ? str[pos] : 0;
    }

    void Parser::advance() {
        pos++;
    }

    bool Parser::finished() {
        return pos >= length;
    }

    void Parser::fail(const std::string &msg) {
        std::cerr << msg << " @" << pos << std::endl;
        _ok = false;
    }

    bool Parser::ok() {
        return _ok;
    }

    bool Parser::match(int c) {
        if (pos < length && str[pos] == c) {
            pos++;
            skipSpace();
            return true;
        }
        return false;
    }

    bool Parser::match(const std::string &s) {
        unsigned long l = s.size();
        if (pos + l >= length) return false;
        for (int i = 0; i < l; ++i) {
            if (s[i] != str[pos + i]) {
                return false;
            }
        }
        pos += l;
        skipSpace();
        return true;
    }

    bool Parser::matchId(const std::string &s, bool advance) {
        unsigned long l = s.size();
        if (pos + l >= length) return false;
        for (int i = 0; i < l; ++i) {
            if (s[i] != str[pos + i]) {
                return false;
            }
        }
        if (pos + l + 1 < length && !isNameChar(str[pos + l + 1]))
            return false;
        if (advance) {
            pos += l;
            skipSpace();
        }
        return true;
    }

    void Parser::skipSpace() {
        while (pos < length) {
            int c = str[pos];
            if (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
                pos++;
            } else {
                break;
            }
        }
    }

    Environ::Environ() { }

    Environ::Environ(Environ &env) {
        for (std::pair<std::string *, Ast *> *pair: env.list) {
            list.push_back(new std::pair<std::string *, Ast *>(pair->first, pair->second));
        }
    }

    void Environ::put(std::string *key, Ast *ast) {
        list.push_front(new std::pair<std::string *, Ast *>(key, ast));
    }

    Ast *Environ::get(std::string *key, bool &ok) {
        for (std::pair<std::string *, Ast *> *pair: list) {
            if (key->compare(*(pair->first)) == 0) {
                ok = true;
                return pair->second;
            }
        }
        ok = false;
        return 0;
    }

    void Environ::unput() {
        delete list[0];
        list.pop_front();
    }

    Environ::~Environ() {
        while (!list.empty()) {
            unput();
        }
    }

    Ast *Ast::operator()(Environ &env, bool &ok) {
        std::cerr << "Unimplemented" << std::endl;
        ok = false;
        return NULL;
    }

    Ast::Ast(Ast_type _type) : type(_type) { }

    Ast::~Ast() { }

    void Ast::print(std::ostream &os) const { }

    Num::Num(int x) : Ast(NUM), num(x) { }

    void Num::print(std::ostream &os) const {
        os << num;
    }

    Ast *Num::operator()(Environ &env, bool &ok) {
        ok = true;
        return this;
    }

    Var::Var(std::string *s) : Ast(VARIABLE), name(s) { }

    Ast *Var::operator()(Environ &env, bool &ok) {
        Ast *ret = env.get(name, ok);
        if (!ok) {
            std::cerr << "Variable not found: '" << *name << "'" << std::endl;
        }
        return ret;
    }

    Var::~Var() {
        delete name;
    }

    void Var::print(std::ostream &os) const {
        os << *name;
    }

    Application::Application(Ast *f, Ast *a) : Ast(APPLICATION), fun(f), arg(a) { }

    Ast *Application::operator()(Environ &env, bool &ok) {
        Ast *fun_val = (*fun)(env, ok);
        if (!ok) return NULL;
        if (Lambda *lambda = dynamic_cast<Lambda *>(fun_val)) {
            return lambda->apply(env, ok, arg);
        }
        std::cerr << "Cannot apply to non-lambda" << std::endl;
        ok = false;
        return NULL;
    }

    Application::~Application() {
        delete fun;
        delete arg;
    }

    void Application::print(std::ostream &os) const {
        os << '(' << *fun << ' ' << *arg << ')';
    }

    Let::Let(std::string *x, Ast *v, Ast *e) : Ast(LET), var(x), value(v), expr(e) { }

    Ast *Let::operator()(Environ &env, bool &ok) {
        (*value)(env, ok);
        if (!ok) {
            return NULL;
        }
        env.put(var, value);
        Ast *ret = (*expr)(env, ok);
        env.unput();
        return ret;
    }

    Let::~Let() {
        delete var;
        delete value;
        delete expr;
    }

    void Let::print(std::ostream &os) const {
        os << "(let " << *var << " = " << *value << " in " << *expr << ')';
    }

    Lambda::Lambda(std::string *v, Ast *e) : Ast(LAMBDA), var(v), expr(e), env(NULL) { }

    Ast *Lambda::operator()(Environ &_env, bool &ok) {
        env = new Environ(_env);
        ok = true;
        return this;
    }

    Ast *Lambda::apply(Environ &_env, bool &ok, Ast *arg) {
        if (!env) {
            std::cerr << "No environment" << std::endl;
            ok = false;
            return NULL;
        }
        (*arg)(_env, ok);
        if (!ok) {
            return NULL;
        }
        env->put(var, arg);
        Ast *ret = (*expr)(*env, ok);
        env->unput();
        return ret;
    }

    Lambda::~Lambda() {
        delete var;
        delete expr;
    }

    void Lambda::print(std::ostream &os) const {
        os << "(\\" << *var << " -> " << *expr << ')';
    }

    std::ostream &operator<<(std::ostream &os, const Ast &ast) {
        ast.print(os);
        return os;
    }

    Ast *parseNum(Parser &parser) {
        int x = 0;
        int c = parser.peek();
        while (isNum(c)) {
            x = x * 10 + (c - '0');
            parser.advance();
            c = parser.peek();
        }
        parser.skipSpace();

        return new Num(x);
    }

    std::string *parseName(Parser &parser) {
        unsigned long start = parser.pos;
        while (isNameChar(parser.peek())) {
            parser.advance();
        }
        std::string *name = new std::string(parser.str.begin() + start, parser.str.begin() + parser.pos);
        parser.skipSpace();
        return name;
    }

    Ast *parseBracket(Parser &parser) {
        Ast *ret = parseExpr(parser);
        if (!parser.ok()) return 0;
        if (!parser.match(')')) {
            delete ret;
            parser.fail("Expected ')'");
            return 0;
        }
        parser.skipSpace();
        return ret;
    }

    Ast *parseLet(Parser &parser) {
        if (!isNameChar(parser.peek())) {
            parser.fail("Let expected identifier");
            return 0;
        }
        std::string *var = parseName(parser);
        if (!parser.match('=')) {
            delete var;
            parser.fail("Let expected '='");
            return 0;
        }
        Ast *val = parseExpr(parser);
        if (!parser.ok()) {
            delete var;
            parser.fail("Let expected expression");
            return 0;
        }
        if (!parser.matchId("in", true)) {
            delete var;
            delete val;
            parser.fail("Let expected in");
            return 0;
        }
        Ast *expr = parseExpr(parser);
        if (!parser.ok()) {
            delete var;
            delete val;
            parser.fail("Let expected in-expression");
            return 0;
        }

        return new Let(var, val, expr);
    }

    Ast *parseLambda(Parser &parser) {
        if (!isNameChar(parser.peek())) {
            parser.fail("Lambda expected identifier");
            return 0;
        }
        std::string *var = parseName(parser);
        if (!parser.match("->")) {
            delete var;
            parser.fail("Lambda expected ->");
            return 0;
        }
        Ast *expr = parseExpr(parser);
        return new Lambda(var, expr);
    }

    Ast *parseExpr(Parser &parser) {
        Ast *ast = NULL, *arg;
        while (!parser.finished()) {
            int c = parser.peek();
            if (isNum(c)) {
                arg = parseNum(parser);
            } else if (isNameChar(c)) {
                if (parser.matchId("let", true)) {
                    arg = parseLet(parser);
                } else if (parser.matchId("in", false)) {
                    break;
                } else {

                    arg = new Var(parseName(parser));
                }
            } else if (parser.match('(')) {
                arg = parseBracket(parser);
            } else if (parser.match('\\')) {
                arg = parseLambda(parser);
            } else {
                break;
            }
            if (!parser.ok()) break;
            if (ast) {
                ast = new Application(ast, arg);
            } else {
                ast = arg;
            }
        }
        if (ast == NULL) {
            parser.fail("Expected expression");
        }
        return ast;
    }
}

int main() {
    using namespace jit;

    std::string prog;

#define BUFFER_SIZE 256
    char buffer[BUFFER_SIZE];

    while (std::cin.getline(buffer, BUFFER_SIZE)) {
        prog = buffer;
        Parser parser(prog);

        Ast *ast = parseExpr(parser);
        if (parser.ok()) {
            Environ env;
            bool ok;
            Ast *ret = (*ast)(env, ok);
            if (ret) {
                std::cout << *ret << std::endl;
            } else {
                std::cout << "error" << std::endl;
            }
        }
    }

    return 0;
}