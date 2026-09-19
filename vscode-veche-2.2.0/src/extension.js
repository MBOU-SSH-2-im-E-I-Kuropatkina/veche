const vscode = require('vscode');

// --- Статические списки для автодополнения ---
const KEYWORDS = [
    'если', 'иначе', 'то', 'пока', 'для', 'в', 'делать',
    'прервать', 'продолжить', 'возврат',
    'функция', 'класс', 'наследует', 'переопределить',
    'неизменяемый', 'изменяемый', 'пустой',
    'новый', 'создать', 'сам', 'базовый',
    'попытка', 'перехват', 'наконец', 'поднять',
    'пусть', 'будет', 'станет', 'постоянное', 'строгое',
    'и', 'или', 'не'
];

const TYPES = [
    'целое', 'дробь', 'строка', 'символ', 'слово',
    'булево', 'список', 'словарь', 'кортеж', 'ничто', 'ничего'
];

const CONSTANTS = ['истина', 'ложь', 'ничто', 'ничего'];

const BUILTINS = ['вывод', 'ввод'];

const GRAPHICS = [
    'окно', 'рисовать_точку', 'рисовать_линию', 'рисовать_прямоугольник',
    'цвет', 'очистить', 'пауза', 'закрыть_окно'
];

const ERROR_TYPES = [
    'ОшибкаСинтаксиса', 'ОшибкаТипа', 'ОшибкаИмени',
    'ОшибкаДеленияНаНоль', 'ОшибкаИндекса', 'ОшибкаВвода'
];

function item(label, kind, detail) {
    const it = new vscode.CompletionItem(label, kind);
    if (detail) it.detail = detail;
    return it;
}

// Собираем имена, объявленные в документе, простыми регекспами.
function collectUserSymbols(document) {
    const text = document.getText();
    const found = new Set();

    for (const m of text.matchAll(/\bфункция\s+([A-Za-zА-Яа-яЁё_][A-Za-zА-Яа-яЁё0-9_]*)/g)) {
        found.add(m[1]);
    }
    for (const m of text.matchAll(/\bкласс\s+([A-Za-zА-Яа-яЁё_][A-Za-zА-Яа-яЁё0-9_]*)/g)) {
        found.add(m[1]);
    }
    const typeAlt = '(?:целое|дробь|строка|символ|слово|булево|список|словарь|кортеж|ничто|ничего)';
    const reVar = new RegExp('\\b' + typeAlt + '\\s+([A-Za-zА-Яа-яЁё_][A-Za-zА-Яа-яЁё0-9_]*)', 'g');
    for (const m of text.matchAll(reVar)) {
        found.add(m[1]);
    }
    for (const m of text.matchAll(/\bдля\s*\(\s*([A-Za-zА-Яа-яЁё_][A-Za-zА-Яа-яЁё0-9_]*)\s+в\b/g)) {
        found.add(m[1]);
    }
    for (const m of text.matchAll(/\bпусть\s+[A-Za-zА-Яа-яЁё_][A-Za-zА-Яа-яЁё0-9_]*\s+([A-Za-zА-Яа-яЁё_][A-Za-zА-Яа-яЁё0-9_]*)\s+будет\b/g)) {
        found.add(m[1]);
    }
    return [...found];
}

function activate(context) {
    const provider = vscode.languages.registerCompletionItemProvider(
        'veche',
        {
            provideCompletionItems(document) {
                const items = [];

                for (const kw of KEYWORDS) {
                    const it = item(kw, vscode.CompletionItemKind.Keyword, 'ключевое слово');
                    it.sortText = '1_' + kw;
                    items.push(it);
                }
                for (const t of TYPES) {
                    const it = item(t, vscode.CompletionItemKind.TypeParameter, 'тип');
                    it.sortText = '2_' + t;
                    items.push(it);
                }
                for (const c of CONSTANTS) {
                    const it = item(c, vscode.CompletionItemKind.Constant, 'константа');
                    it.sortText = '3_' + c;
                    items.push(it);
                }
                for (const b of BUILTINS) {
                    const it = item(b, vscode.CompletionItemKind.Function, 'встроенная функция');
                    it.sortText = '4_' + b;
                    items.push(it);
                }
                for (const g of GRAPHICS) {
                    const it = item(g, vscode.CompletionItemKind.Function, 'графическая функция');
                    it.sortText = '5_' + g;
                    items.push(it);
                }
                for (const e of ERROR_TYPES) {
                    const it = item(e, vscode.CompletionItemKind.Class, 'тип ошибки');
                    it.sortText = '6_' + e;
                    items.push(it);
                }
                for (const name of collectUserSymbols(document)) {
                    const it = item(name, vscode.CompletionItemKind.Variable, 'объявлено в файле');
                    it.sortText = '7_' + name;
                    items.push(it);
                }
                return items;
            }
        },
        '.'
    );
    context.subscriptions.push(provider);
}

function deactivate() {}

module.exports = { activate, deactivate };