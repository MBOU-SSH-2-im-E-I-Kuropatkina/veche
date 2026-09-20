#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "lexer.h"
#include "parser.h"
#include "analyzer.h"
#include "interpreter.h"
#include "errors.h"
#include "graphics.h"

#ifdef _WIN32
#  include <windows.h>
#endif

using namespace veche;

static std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Не удалось открыть файл: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string resolvePath(const std::string& arg) {
    auto ends = [](const std::string& s, const std::string& suf) {
        return s.size() >= suf.size()
            && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
    };
    if (ends(arg, ".veche") || ends(arg, ".vech")) return arg;
    std::ifstream f1(arg + ".veche");
    if (f1) return arg + ".veche";
    std::ifstream f2(arg + ".vech");
    if (f2) return arg + ".vech";
    return arg;
}

// Лексер + парсер → программа. Бросает VecheError при синтаксисе.
static std::vector<StmtPtr> parseSource(const std::string& src,
                                        const std::string& file,
                                        std::vector<std::string>& linesOut) {
    Lexer lex(src, file);
    auto toks = lex.tokenize();
    linesOut = lex.lines();
    Parser parser(std::move(toks), file, &linesOut);
    return parser.parseProgram();
}

// Запуск файла — новый Interpreter, новый Scope.
static int runFile(const std::string& src, const std::string& file) {
    try {
        std::vector<std::string> lines;
        auto prog = parseSource(src, file, lines);

        Analyzer an;
        an.analyze(prog);

        Interpreter interp;
        interp.setSource(file, lines);
        interp.run(prog);

        graphics::closeWindow();
        return 0;
    } catch (VecheError& e) {
        if (e.file.empty()) e.file = file;
        printError(e);
        graphics::closeWindow();
        return 1;
    } catch (std::exception& e) {
        std::cerr << "[Вече: ОшибкаВнутренняя] " << e.what() << "\n";
        graphics::closeWindow();
        return 2;
    }
}

// REPL: один Interpreter и один Scope на всю сессию.
static int repl() {
    std::cout << "Вече REPL. Введите 'выход' для завершения.\n";

    Interpreter interp;
    ScopePtr    env = interp.makeGlobalScope();
    interp.setSource("<repl>", {});

    std::string line;
    while (true) {
        std::cout << "вече> ";
        std::cout.flush();
        if (!std::getline(std::cin, line)) break;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line == "выход" || line == "exit" || line == "quit") break;
        if (line.empty()) continue;

        try {
            std::vector<std::string> lines;
            auto prog = parseSource(line, "<repl>", lines);
            interp.setSource("<repl>", lines);
            interp.runIn(prog, env);
        } catch (VecheError& e) {
            if (e.line <= 0) e.line = 1;
            if (e.col  <= 0) e.col  = 1;
            e.file = "<repl>";
            printError(e);
        } catch (std::exception& ex) {
            std::cerr << "[Вече: ОшибкаВнутренняя] " << ex.what() << "\n";
        }
    }
    return 0;
}

static void printHelp() {
    std::cout <<
        "Вече — интерпретатор кириллического языка программирования.\n"
        "\n"
        "Использование:\n"
        "  veche <файл>     запустить файл (.veche / .vech подставляется)\n"
        "  veche -i         REPL\n"
        "  veche -r         REPL (синоним)\n"
        "  veche -v         версия\n"
        "  veche -h         помощь\n";
}

static void printVersion() {
    std::cout << "Вече 2.1.5 (C++17, TDM-GCC-64, GDI32-графика)\n";
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if (argc < 2) { printHelp(); return 0; }
    std::string a = argv[1];
    if (a == "-h" || a == "--help")    { printHelp(); return 0; }
    if (a == "-v" || a == "--version") { printVersion(); return 0; }
    if (a == "-i" || a == "-r")        return repl();

    std::string path = resolvePath(a);
    try {
        std::string src = readFile(path);
        return runFile(src, path);
    } catch (std::exception& e) {
        std::cerr << "[Вече: ОшибкаВвода] " << e.what() << "\n";
        return 1;
    }
}