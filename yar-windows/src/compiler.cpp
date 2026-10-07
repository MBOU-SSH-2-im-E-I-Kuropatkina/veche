#include "compiler.h"
#include <iostream>
#include <fstream>
#include <sstream>

// LLVM заголовки (только здесь, чтобы не замедлять сборку)
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Bitcode/BitcodeWriter.h>

namespace veche {

// ============================================================================
// Конструктор / Деструктор
// ============================================================================

Compiler::Compiler() {
    // Инициализация таргетов LLVM
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("veche_program", *context);
    builder = std::make_unique<llvm::IRBuilder<true>>(*context);
}

Compiler::~Compiler() = default;

// ============================================================================
// Управление областями видимости
// ============================================================================

void Compiler::enterScope() {
    symbolTables.push_back(std::unordered_map<std::string, llvm::AllocaInst*>());
}

void Compiler::exitScope() {
    if (!symbolTables.empty()) {
        symbolTables.pop_back();
    }
}

void Compiler::setVariable(const std::string& name, llvm::AllocaInst* alloca) {
    if (!symbolTables.empty()) {
        symbolTables.back()[name] = alloca;
    }
}

llvm::AllocaInst* Compiler::getVariable(const std::string& name) {
    // Ищем переменную от текущей области к внешней
    for (auto it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return nullptr;
}

llvm::AllocaInst* Compiler::createEntryBlockAlloca(llvm::Function* func, 
                                                     const std::string& varName, 
                                                     llvm::Type* type) {
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, varName.c_str());
}

// ============================================================================
// Преобразование типов
// ============================================================================

llvm::Type* Compiler::getLLVMType(const std::string& typeName) {
    if (typeName == "целое") return llvm::Type::getInt64Ty(*context);
    if (typeName == "дробь") return llvm::Type::getDoubleTy(*context);
    if (typeName == "булево") return llvm::Type::getInt1Ty(*context);
    if (typeName == "символ") return llvm::Type::getInt32Ty(*context); // UTF-32 codepoint
    if (typeName == "строка") return llvm::PointerType::getUnqual(*context); // i8*
    if (typeName == "ничто" || typeName == "ничего") return llvm::Type::getVoidTy(*context);
    
    // По умолчанию целое
    return llvm::Type::getInt64Ty(*context);
}

// ============================================================================
// Встроенные функции (printf, scanf, и runtime функции Вече)
// ============================================================================

llvm::Function* Compiler::getOrInsertPrintf() {
    llvm::Function* func = module->getFunction("printf");
    if (!func) {
        llvm::PointerType* ptrType = llvm::PointerType::getUnqual(*context);
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*context), {ptrType}, true
        );
        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "printf", module.get());
    }
    return func;
}

llvm::Function* Compiler::getOrInsertScanf() {
    llvm::Function* func = module->getFunction("scanf");
    if (!func) {
        llvm::PointerType* ptrType = llvm::PointerType::getUnqual(*context);
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*context), {ptrType}, true
        );
        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "scanf", module.get());
    }
    return func;
}

llvm::Function* Compiler::getOrInsertVechePrint() {
    // Внешняя функция из runtime: void veche_print_int64(int64_t)
    llvm::Function* func = module->getFunction("veche_print_int64");
    if (!func) {
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context), {llvm::Type::getInt64Ty(*context)}, false
        );
        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "veche_print_int64", module.get());
    }
    return func;
}

llvm::Function* Compiler::getOrInsertVecheInput() {
    // Внешняя функция: int64_t veche_input_int64()
    llvm::Function* func = module->getFunction("veche_input_int64");
    if (!func) {
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(*context), {}, false
        );
        func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "veche_input_int64", module.get());
    }
    return func;
}

// ============================================================================
// Компиляция выражений
// ============================================================================

llvm::Value* Compiler::compileExpr(const ExprPtr& expr) {
    if (!expr) return nullptr;

    switch (expr->kind) {
        case ExprKind::Literal: {
            if (!expr->literal) return nullptr;
            
            switch (expr->literal->type) {
                case ValueType::Integer:
                    return llvm::ConstantInt::get(*context, 
                        llvm::APInt(64, expr->literal->intVal, true));
                
                case ValueType::Double:
                    return llvm::ConstantFP::get(*context, 
                        llvm::APFloat(expr->literal->doubleVal));
                
                case ValueType::Boolean:
                    return llvm::ConstantInt::get(*context, 
                        llvm::APInt(1, expr->literal->boolVal ? 1 : 0));
                
                case ValueType::String: {
                    // Создаем глобальную строку
                    return builder->CreateGlobalStringPtr(expr->literal->strVal, "str");
                }
                
                default:
                    std::cerr << "Неподдерживаемый тип литерала\n";
                    return nullptr;
            }
        }

        case ExprKind::Variable: {
            llvm::AllocaInst* alloca = getVariable(expr->name);
            if (!alloca) {
                std::cerr << "Ошибка: неизвестная переменная '" << expr->name << "'\n";
                return nullptr;
            }
            return builder->CreateLoad(alloca->getAllocatedType(), alloca, expr->name.c_str());
        }

        case ExprKind::Binary: {
            llvm::Value* left = compileExpr(expr->left);
            llvm::Value* right = compileExpr(expr->right);
            if (!left || !right) return nullptr;

            // Определяем тип операции
            bool isFloat = left->getType()->isDoubleTy() || right->getType()->isDoubleTy();

            if (expr->op == "+") {
                if (isFloat) {
                    left = builder->CreateFPExt(left, llvm::Type::getDoubleTy(*context));
                    right = builder->CreateFPExt(right, llvm::Type::getDoubleTy(*context));
                    return builder->CreateFAdd(left, right, "addtmp");
                }
                return builder->CreateAdd(left, right, "addtmp");
            }
            if (expr->op == "-") {
                if (isFloat) return builder->CreateFSub(left, right, "subtmp");
                return builder->CreateSub(left, right, "subtmp");
            }
            if (expr->op == "*") {
                if (isFloat) return builder->CreateFMul(left, right, "multmp");
                return builder->CreateMul(left, right, "multmp");
            }
            if (expr->op == "/") {
                if (isFloat) return builder->CreateFDiv(left, right, "divtmp");
                return builder->CreateSDiv(left, right, "divtmp");
            }
            if (expr->op == "%") {
                return builder->CreateSRem(left, right, "remtmp");
            }
            
            // Операторы сравнения
            if (expr->op == "==") {
                if (isFloat) return builder->CreateFCmpOEQ(left, right, "eqtmp");
                return builder->CreateICmpEQ(left, right, "eqtmp");
            }
            if (expr->op == "!=") {
                if (isFloat) return builder->CreateFCmpONE(left, right, "netmp");
                return builder->CreateICmpNE(left, right, "netmp");
            }
            if (expr->op == "<") {
                if (isFloat) return builder->CreateFCmpOLT(left, right, "lttmp");
                return builder->CreateICmpSLT(left, right, "lttmp");
            }
            if (expr->op == ">") {
                if (isFloat) return builder->CreateFCmpOGT(left, right, "gttmp");
                return builder->CreateICmpSGT(left, right, "gttmp");
            }
            if (expr->op == "<=") {
                if (isFloat) return builder->CreateFCmpOLE(left, right, "letmp");
                return builder->CreateICmpSLE(left, right, "letmp");
            }
            if (expr->op == ">=") {
                if (isFloat) return builder->CreateFCmpOGE(left, right, "getmp");
                return builder->CreateICmpSGE(left, right, "getmp");
            }

            // Логические операторы
            if (expr->op == "&&") {
                return builder->CreateAnd(left, right, "andtmp");
            }
            if (expr->op == "||") {
                return builder->CreateOr(left, right, "ortmp");
            }

            std::cerr << "Неподдерживаемый оператор: " << expr->op << "\n";
            return nullptr;
        }

        case ExprKind::Unary: {
            llvm::Value* operand = compileExpr(expr->right);
            if (!operand) return nullptr;

            if (expr->op == "-") {
                if (operand->getType()->isDoubleTy()) {
                    return builder->CreateFNeg(operand, "negtmp");
                }
                return builder->CreateNeg(operand, "negtmp");
            }
            if (expr->op == "!") {
                return builder->CreateNot(operand, "nottmp");
            }

            std::cerr << "Неподдерживаемый унарный оператор: " << expr->op << "\n";
            return nullptr;
        }

        case ExprKind::Call: {
            // Получаем имя функции
            std::string funcName;
            if (expr->callee->kind == ExprKind::Variable) {
                funcName = expr->callee->name;
            } else {
                std::cerr << "Поддерживаются только прямые вызовы функций\n";
                return nullptr;
            }

            // Компилируем аргументы
            std::vector<llvm::Value*> args;
            for (const auto& arg : expr->args) {
                llvm::Value* argVal = compileExpr(arg);
                if (!argVal) return nullptr;
                args.push_back(argVal);
            }

            // Встроенные функции
            if (funcName == "вывод") {
                if (args.empty()) return nullptr;
                
                llvm::Value* arg = args[0];
                
                // Если это целое число
                if (arg->getType()->isIntegerTy(64)) {
                    llvm::Function* printFunc = getOrInsertVechePrint();
                    return builder->CreateCall(printFunc, {arg});
                }
                
                // Если это строка
                if (arg->getType()->isPointerTy()) {
                    llvm::Function* printfFunc = getOrInsertPrintf();
                    llvm::Value* formatStr = builder->CreateGlobalStringPtr("%s\n", "fmt_str");
                    return builder->CreateCall(printfFunc, {formatStr, arg});
                }
                
                // Для дробных чисел
                if (arg->getType()->isDoubleTy()) {
                    llvm::Function* printfFunc = getOrInsertPrintf();
                    llvm::Value* formatStr = builder->CreateGlobalStringPtr("%f\n", "fmt_dbl");
                    return builder->CreateCall(printfFunc, {formatStr, arg});
                }
                
                std::cerr << "Неподдерживаемый тип для вывода\n";
                return nullptr;
            }

            if (funcName == "ввод") {
                // Пока поддерживаем только ввод целых
                llvm::Function* inputFunc = getOrInsertVecheInput();
                return builder->CreateCall(inputFunc, {});
            }

            // Пользовательская функция
            llvm::Function* func = module->getFunction(funcName);
            if (!func) {
                std::cerr << "Неизвестная функция: " << funcName << "\n";
                return nullptr;
            }

            return builder->CreateCall(func, args, "calltmp");
        }

        default:
            std::cerr << "Неподдерживаемое выражение (kind=" 
                      << static_cast<int>(expr->kind) << ")\n";
            return nullptr;
    }
}

// ============================================================================
// Компиляция операторов
// ============================================================================

void Compiler::compileStmt(const StmtPtr& stmt) {
    if (!stmt) return;

    switch (stmt->kind) {
        case StmtKind::VarDecl: {
            llvm::Type* type = getLLVMType(stmt->typeName);
            llvm::AllocaInst* alloca = createEntryBlockAlloca(currentFunction, stmt->varName, type);
            setVariable(stmt->varName, alloca);

            if (stmt->init) {
                llvm::Value* initVal = compileExpr(stmt->init);
                if (initVal) {
                    // Приведение типов, если нужно
                    if (initVal->getType() != type) {
                        if (type->isDoubleTy() && initVal->getType()->isIntegerTy()) {
                            initVal = builder->CreateSIToFP(initVal, type, "cast");
                        } else if (type->isIntegerTy() && initVal->getType()->isDoubleTy()) {
                            initVal = builder->CreateFPToSI(initVal, type, "cast");
                        }
                    }
                    builder->CreateStore(initVal, alloca);
                }
            }
            break;
        }

        case StmtKind::Assign: {
            // Упрощенно: поддерживаем только присваивание переменной
            if (stmt->assignTarget->kind != ExprKind::Variable) {
                std::cerr << "Поддерживается только присваивание простым переменным\n";
                break;
            }

            std::string varName = stmt->assignTarget->name;
            llvm::AllocaInst* alloca = getVariable(varName);
            if (!alloca) {
                std::cerr << "Неизвестная переменная: " << varName << "\n";
                break;
            }

            llvm::Value* val = compileExpr(stmt->assignValue);
            if (val) {
                builder->CreateStore(val, alloca);
            }
            break;
        }

        case StmtKind::If: {
            llvm::Value* condVal = compileExpr(stmt->cond);
            if (!condVal) break;

            // Приводим условие к i1 (bool)
            if (!condVal->getType()->isIntegerTy(1)) {
                if (condVal->getType()->isIntegerTy()) {
                    condVal = builder->CreateICmpNE(condVal, 
                        llvm::ConstantInt::get(condVal->getType(), 0), "ifcond");
                } else if (condVal->getType()->isDoubleTy()) {
                    condVal = builder->CreateFCmpONE(condVal, 
                        llvm::ConstantFP::get(condVal->getType(), 0.0), "ifcond");
                }
            }

            // Создаем базовые блоки
            llvm::Function* func = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context, "then", func);
            llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context, "else");
            llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context, "ifcont");

            builder->CreateCondBr(condVal, thenBB, stmt->elseBranch ? elseBB : mergeBB);

            // Then ветка
            builder->SetInsertPoint(thenBB);
            compileBlock(stmt->body);
            builder->CreateBr(mergeBB);

            // Else ветка
            if (stmt->elseBranch) {
                func->insert(func->end(), elseBB);
                builder->SetInsertPoint(elseBB);
                compileStmt(stmt->elseBranch);
                builder->CreateBr(mergeBB);
            }

            // Merge
            func->insert(func->end(), mergeBB);
            builder->SetInsertPoint(mergeBB);
            break;
        }

        case StmtKind::While: {
            llvm::Function* func = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context, "whilecond", func);
            llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*context, "whilebody");
            llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(*context, "afterwhile");

            builder->CreateBr(condBB);

            // Условие
            builder->SetInsertPoint(condBB);
            llvm::Value* condVal = compileExpr(stmt->cond);
            if (!condVal) break;

            if (!condVal->getType()->isIntegerTy(1)) {
                if (condVal->getType()->isIntegerTy()) {
                    condVal = builder->CreateICmpNE(condVal, 
                        llvm::ConstantInt::get(condVal->getType(), 0), "whilecond");
                }
            }
            builder->CreateCondBr(condVal, bodyBB, afterBB);

            // Тело
            func->insert(func->end(), bodyBB);
            builder->SetInsertPoint(bodyBB);
            compileStmt(stmt->whileBody);
            builder->CreateBr(condBB);

            // После цикла
            func->insert(func->end(), afterBB);
            builder->SetInsertPoint(afterBB);
            break;
        }

        case StmtKind::ForClassic: {
            enterScope();

            // Инициализация
            if (stmt->forInit) {
                compileStmt(stmt->forInit);
            }

            llvm::Function* func = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context, "forcond", func);
            llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*context, "forbody");
            llvm::BasicBlock* stepBB = llvm::BasicBlock::Create(*context, "forstep");
            llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(*context, "afterfor");

            builder->CreateBr(condBB);

            // Условие
            builder->SetInsertPoint(condBB);
            if (stmt->cond) {
                llvm::Value* condVal = compileExpr(stmt->cond);
                if (!condVal) break;

                if (!condVal->getType()->isIntegerTy(1)) {
                    condVal = builder->CreateICmpNE(condVal, 
                        llvm::ConstantInt::get(condVal->getType(), 0), "forcond");
                }
                builder->CreateCondBr(condVal, bodyBB, afterBB);
            } else {
                builder->CreateBr(bodyBB);
            }

            // Тело
            func->insert(func->end(), bodyBB);
            builder->SetInsertPoint(bodyBB);
            compileBlock(stmt->body);
            builder->CreateBr(stepBB);

            // Шаг
            func->insert(func->end(), stepBB);
            builder->SetInsertPoint(stepBB);
            if (stmt->forStep) {
                compileStmt(stmt->forStep);
            }
            builder->CreateBr(condBB);

            // После цикла
            func->insert(func->end(), afterBB);
            builder->SetInsertPoint(afterBB);

            exitScope();
            break;
        }

        case StmtKind::FunctionDecl: {
            // Создаем тип функции
            std::vector<llvm::Type*> paramTypes;
            for (const auto& param : stmt->params) {
                paramTypes.push_back(getLLVMType(param.typeName));
            }

            llvm::Type* returnType = getLLVMType(stmt->returnType);
            llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);

            llvm::Function* func = llvm::Function::Create(
                funcType, llvm::Function::ExternalLinkage, stmt->funcName, module.get()
            );

            // Сохраняем текущую функцию
            llvm::Function* prevFunc = currentFunction;
            currentFunction = func;

            // Создаем entry блок
            llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", func);
            builder->SetInsertPoint(entry);

            enterScope();

            // Создаем аллокации для параметров
            for (size_t i = 0; i < stmt->params.size(); ++i) {
                llvm::Argument* arg = func->getArg(i);
                arg->setName(stmt->params[i].name);
                llvm::AllocaInst* alloca = createEntryBlockAlloca(func, stmt->params[i].name, arg->getType());
                builder->CreateStore(arg, alloca);
                setVariable(stmt->params[i].name, alloca);
            }

            // Компилируем тело функции
            compileStmt(stmt->funcBody);

            // Если функция возвращает void, добавляем ret void
            if (returnType->isVoidTy()) {
                builder->CreateRetVoid();
            }

            exitScope();
            currentFunction = prevFunc;
            break;
        }

        case StmtKind::Return: {
            if (stmt->returnValue) {
                llvm::Value* retVal = compileExpr(stmt->returnValue);
                if (retVal) {
                    builder->CreateRet(retVal);
                }
            } else {
                builder->CreateRetVoid();
            }
            break;
        }

        case StmtKind::Block: {
            enterScope();
            compileBlock(stmt->body);
            exitScope();
            break;
        }

        case StmtKind::Print: {
            // Обработка оператора "вывод(...)"
            if (!stmt->printArgs.empty()) {
                llvm::Value* val = compileExpr(stmt->printArgs[0]);
                if (val) {
                    if (val->getType()->isIntegerTy(64)) {
                        llvm::Function* printFunc = getOrInsertVechePrint();
                        builder->CreateCall(printFunc, {val});
                    } else if (val->getType()->isPointerTy()) {
                        llvm::Function* printfFunc = getOrInsertPrintf();
                        llvm::Value* formatStr = builder->CreateGlobalStringPtr("%s\n", "fmt");
                        builder->CreateCall(printfFunc, {formatStr, val});
                    } else if (val->getType()->isDoubleTy()) {
                        llvm::Function* printfFunc = getOrInsertPrintf();
                        llvm::Value* formatStr = builder->CreateGlobalStringPtr("%f\n", "fmt");
                        builder->CreateCall(printfFunc, {formatStr, val});
                    }
                }
            }
            break;
        }

        default:
            std::cerr << "Неподдерживаемый оператор (тип=" 
                      << static_cast<int>(stmt->kind) << ")\n";
    }
}

void Compiler::compileBlock(const std::vector<StmtPtr>& body) {
    for (const auto& stmt : body) {
        compileStmt(stmt);
    }
}

// ============================================================================
// Главная функция компиляции
// ============================================================================

bool Compiler::compile(const std::vector<StmtPtr>& ast, const std::string& outputPath) {
    // 1. Создаем функцию main
    llvm::FunctionType* mainType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context), false
    );
    llvm::Function* mainFunc = llvm::Function::Create(
        mainType, llvm::Function::ExternalLinkage, "main", module.get()
    );

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", mainFunc);
    builder->SetInsertPoint(entry);

    currentFunction = mainFunc;
    enterScope();

    // 2. Сначала объявляем все функции (чтобы их можно было вызывать)
    for (const auto& stmt : ast) {
        if (stmt->kind == StmtKind::FunctionDecl) {
            // Предварительное объявление
            std::vector<llvm::Type*> paramTypes;
            for (const auto& param : stmt->params) {
                paramTypes.push_back(getLLVMType(param.typeName));
            }
            llvm::Type* returnType = getLLVMType(stmt->returnType);
            llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);
            llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, stmt->funcName, module.get());
        }
    }

    // 3. Компилируем все операторы верхнего уровня
    for (const auto& stmt : ast) {
        compileStmt(stmt);
    }

    // 4. Добавляем return 0 в конец main
    builder->CreateRet(llvm::ConstantInt::get(*context, llvm::APInt(32, 0, true)));

    exitScope();

    // 5. Верификация модуля
    std::string errStr;
    llvm::raw_string_ostream errStream(errStr);
    if (llvm::verifyModule(*module, &errStream)) {
        std::cerr << "Ошибка в LLVM IR:\n" << errStr << "\n";
        return false;
    }

    // 6. Генерация объектного файла
    std::string targetTriple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);

    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Ошибка таргета: " << error << "\n";
        return false;
    }

    llvm::TargetOptions opt;
    auto targetMachine = target->createTargetMachine(
        targetTriple, "generic", "", opt, llvm::Reloc::Model::PIC_
    );

    std::error_code ec;
    llvm::raw_fd_ostream dest(outputPath, ec, llvm::sys::fs::OF_None);
    if (ec) {
        std::cerr << "Не удалось открыть файл: " << ec.message() << "\n";
        return false;
    }

    llvm::legacy::PassManager pass;
    llvm::CodeGenFileType fileType = llvm::CGFT_ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        std::cerr << "Таргет не поддерживает генерацию файла\n";
        return false;
    }

    pass.run(*module);
    dest.flush();

    std::cout << "Успешно сгенерирован объектный файл: " << outputPath << "\n";
    return true;
}

} // namespace veche