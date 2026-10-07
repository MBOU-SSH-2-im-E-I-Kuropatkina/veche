#pragma once
#include "ast.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

// Декларации для LLVM типов (чтобы не тащить тяжелые хедеры)
namespace llvm {
    class LLVMContext;
    class Module;
    class Function;
    class BasicBlock;
    class Value;
    class Type;
    class AllocaInst;
    template <bool> class IRBuilder;
}

namespace veche {

class Compiler {
public:
    Compiler();
    ~Compiler();

    // Компилирует AST в исполняемый файл
    bool compile(const std::vector<StmtPtr>& ast, const std::string& outputPath);

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<true>> builder;

    // Текущая функция, в которой мы находимся
    llvm::Function* currentFunction = nullptr;

    // Таблица символов: имя переменной -> указатель на ячейку памяти (Alloca)
    std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> symbolTables;

    // Множество объявленных функций (чтобы не дублировать)
    std::unordered_set<std::string> declaredFunctions;

    // Управление областями видимости
    void enterScope();
    void exitScope();
    void setVariable(const std::string& name, llvm::AllocaInst* alloca);
    llvm::AllocaInst* getVariable(const std::string& name);

    // Создание аллокации в entry блоке текущей функции
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func, 
                                              const std::string& varName, 
                                              llvm::Type* type);

    // Преобразование в тип LLVM
    llvm::Type* getLLVMType(const std::string& typeName);

    // Компиляция выражений и операторов
    llvm::Value* compileExpr(const ExprPtr& expr);
    void compileStmt(const StmtPtr& stmt);
    void compileBlock(const std::vector<StmtPtr>& body);

    // Вспомогательные функции для встроенных операций
    llvm::Function* getOrInsertPrintf();
    llvm::Function* getOrInsertScanf();
    llvm::Function* getOrInsertVechePrint();
    llvm::Function* getOrInsertVecheInput();
};

} // namespace veche