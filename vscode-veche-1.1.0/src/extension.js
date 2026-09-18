const vscode = require('vscode');

// --- Статические списки ---
const KEYWORDS = [
    'если', 'иначе', 'пока', 'для', 'в',
    'прервать', 'продолжить', 'возврат',
    'функция', 'класс', 'наследует', 'переопределить',
    'новый', 'себя', 'базовый',
    'попытка', 'перехват', 'наконец', 'поднять',
    'константа', 'строгое',
    'и', 'или', 'не'
];

const TYPES = [
    'целое', 'дробное', 'слово', 'строка',
    'булево', 'список', 'словарь', 'кортеж', 'ничто'
];

const CONSTANTS = ['истина', 'ложь', 'ничто'];

const BUILTINS = ['печать', 'ввод'];

const ERROR_TYPES = [
    'ОшибкаСинтаксиса', 'ОшибкаТипа', 'ОшибкаИмени',
    'ОшибкаДеленияНаНоль', 'ОшибкаИндекса', 'ОшибкаВвода'
];

function item(label, kind, detail) {
    const it = new vscode.CompletionItem(label, kind);
    if (detail) it.detail = detail;
    return it;
}

// Собрать имена, объявленные в документе
function collectUserSymbols(document) {
    const text = document.getText();
    const found = new Set();

    // функция <имя>(
    for (const m of text.matchAll(/\bфункция\s+([A-Za-zА-Яа-яЁё_][A-Za-zА-Яа-яЁё0-9_]*)/g)) {
        found.add(m[1]);
    }
    // класс <Имя>
    for (const m of text.matchAll(/\bкласс\s+([A-Za-zА-Яа-яЁё_][A-Za-zА-Яа-яЁё0-9_]*)/g)) {
        found.add(m[1]);
    }
    // <тип> <имя>   — объявление переменной/поля/параметра
    const typeAlt = '(?:целое|дробное|слово|строка|булево|список|словарь|кортеж|ничто)';
    const reVar = new RegExp('\\b' + typeAlt + '\\s+([A-Za-zА-Яа-яЁё_][A-Za-zА-Яа-яЁё0-9_]*)', 'g');
    for (const m of text.matchAll(reVar)) {
        found.add(m[1]);
    }
    // для (<имя> в ...
    for (const m of text.matchAll(/\bдля\s*\(\s*([A-Za-zА-Яа-яЁё_][A-Za-zА-Яа-яЁё0-9_]*)\s+в\b/g)) {
        found.add(m[1]);
    }

    return [...found];
}

function activate(context) {
    const provider = vscode.languages.registerCompletionItemProvider(
        'veche',
        {
            provideCompletionItems(document, position) {
                const items = [];

                // 1. Ключевые слова
                for (const kw of KEYWORDS) {
                    const it = item(kw, vscode.CompletionItemKind.Keyword, 'ключевое слово');
                    it.sortText = '1_' + kw;
                    items.push(it);
                }

                // 2. Типы
                for (const t of TYPES) {
                    const it = item(t, vscode.CompletionItemKind.TypeParameter, 'тип');
                    it.sortText = '2_' + t;
                    items.push(it);
                }

                // 3. Константы
                for (const c of CONSTANTS) {
                    const it = item(c, vscode.CompletionItemKind.Constant, 'константа');
                    it.sortText = '3_' + c;
                    items.push(it);
                }

                // 4. Встроенные функции
                for (const b of BUILTINS) {
                    const it = item(b, vscode.CompletionItemKind.Function, 'встроенная функция');
                    it.sortText = '4_' + b;
                    items.push(it);
                }

                // 5. Типы ошибок
                for (const e of ERROR_TYPES) {
                    const it = item(e, vscode.CompletionItemKind.Class, 'тип ошибки');
                    it.sortText = '5_' + e;
                    items.push(it);
                }

                // 6. Пользовательские символы из документа
                for (const name of collectUserSymbols(document)) {
                    const it = item(name, vscode.CompletionItemKind.Variable, 'объявлено в файле');
                    it.sortText = '6_' + name;
                    items.push(it);
                }

                return items;
            }
        },
        // триггерные символы: подсказывать сразу после этих знаков
        // (пробел не входит — иначе будет спамить)
        '.'
    );

    context.subscriptions.push(provider);
}

function deactivate() {}

module.exports = { activate, deactivate };