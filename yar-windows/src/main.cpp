#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include "lexer.h"
#include "parser.h"
#include "analyzer.h"
#include "interpreter.h"
#include "errors.h"
#include "compiler.h"  // НОВОЕ: подключаем компилятор

namespace fs = std::filesystem;

void printVersion() {
    std::cout << "Вече версия 2.5.1\n";
    std::cout << "Интерпретатор и компилятор кириллического языка программирования\n";
}

void printHelp() {
    std::cout << "Использование:\n";
    std::cout << "  veche <файл>              Запустить файл (.veche / .vech)\n";
    std::cout << "  veche -c <файл>           Компилировать в объектный файл\n";
    std::cout << "  veche -c <файл> -o <out>  Компилировать с указанием имени выхода\n";
    std::cout << "  veche -i                  REPL (интерактивный режим)\n";
    std::cout << "  veche -r                  REPL (синоним)\n";
    std::cout << "  veche -v                  Версия\n";
    std::cout << "  veche -h                  Помощь\n";
}

std::string findFileWithExtension(const std::string& filename) {
    // Если файл существует как есть - возвращаем его
    if (fs::exists(filename)) {
        return filename;
    }
    
    // Пробуем добавить .veche
    std::string withVeche = filename + ".veche";
    if (fs::exists(withVeche)) {
        return withVeche;
    }
    
    // Пробуем добавить .vech
    std::string withVech = filename + ".vech";
    if (fs::exists(withVech)) {
        return withVech;
    }
    
    // Ничего не нашли
    return filename;
}

void runREPL() {
    std::cout << "Вече REPL v2.5.1\n";
    std::cout << "Введите 'выход' или Ctrl+C для выхода\n\n";
    
    veche::Interpreter interpreter;
    std::string line;
    
    while (true) {
        std::cout << ">>> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        if (line == "выход" || line == "exit") {
            break;
        }
        
        if (line.empty()) {
            continue;
        }
        
        try {
            veche::Lexer lexer(line, "<repl>");
            auto tokens = lexer.tokenize();
            
            veche::Parser parser(tokens, "<repl>");
            auto ast = parser.parse();
            
            veche::Analyzer analyzer;
            analyzer.analyze(ast);
            
            interpreter.run(ast);
        } catch (const veche::VecheError& e) {
            std::cerr << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Ошибка: " << e.what() << "\n";
        }
    }
}

int main(int argc, char** argv) {
    // Парсим аргументы командной строки
    bool compile_mode = false;
    bool repl_mode = false;
    std::string output_file = "output.obj";
    std::string input_file;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-c") {
            compile_mode = true;
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "-i" || arg == "-r") {
            repl_mode = true;
        } else if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        } else if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg[0] != '-') {
            input_file = arg;
        } else {
            std::cerr << "Неизвестный флаг: " << arg << "\n";
            printHelp();
            return 1;
        }
    }
    
    // Режим REPL
    if (repl_mode) {
        runREPL();
        return 0;
    }
    
    // Если файл не указан
    if (input_file.empty()) {
        std::cerr << "Ошибка: не указан входной файл\n";
        printHelp();
        return 1;
    }
    
    // Ищем файл с правильным расширением
    std::string actual_file = findFileWithExtension(input_file);
    
    if (!fs::exists(actual_file)) {
        std::cerr << "Ошибка: файл не найден: " << input_file << "\n";
        return 1;
    }
    
    // Читаем файл
    std::ifstream file(actual_file);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл: " << actual_file << "\n";
        return 1;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source_code = buffer.str();
    file.close();
    
    try {
        // Лексический анализ
        veche::Lexer lexer(source_code, actual_file);
        auto tokens = lexer.tokenize();
        
        // Синтаксический анализ
        veche::Parser parser(tokens, actual_file);
        auto ast = parser.parse();
        
        // Семантический анализ
        veche::Analyzer analyzer;
        analyzer.analyze(ast);
        
        // Режим компиляции
        if (compile_mode) {
            veche::Compiler compiler;
            if (compiler.compile(ast, output_file)) {
                std::cout << "✓ Компиляция завершена успешно!\n";
                std::cout << "Объектный файл: " << output_file << "\n\n";
                std::cout << "Для создания исполняемого файла выполните:\n";
                std::cout << "  1. Скомпилируйте runtime:\n";
                std::cout << "     clang -c veche_runtime.c -o veche_runtime.obj\n\n";
                std::cout << "  2. Слинкуйте:\n";
                std::cout << "     lld-link " << output_file << " veche_runtime.obj /OUT:program.exe\n\n";
                std::cout << "  3. Запустите:\n";
                std::cout << "     program.exe\n";
            } else {
                std::cerr << "✗ Ошибка компиляции\n";
                return 1;
            }
        } else {
            // Режим интерпретации
            veche::Interpreter interpreter;
            interpreter.run(ast);
        }
        
    } catch (const veche::VecheError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}