#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ======================
// Дерево выражения
// ======================
typedef struct Node {
    char op;              // '+', '*', или 0 (если число или переменная)
    char val[32];         // если переменная — её имя
    struct Node *left;
    struct Node *right;
} Node;

// ======================
// Создание узлов дерева
// ======================
Node* create_node(char op, const char* val, Node* left, Node* right) {
    Node* node = malloc(sizeof(Node));
    node->op = op;
    if (val) strncpy(node->val, val, 31);
    else node->val[0] = 0;
    node->left = left;
    node->right = right;
    return node;
}

void free_tree(Node* root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

// ======================
// Простой рекурсивный парсер выражения
// ======================
const char* expr;

void skip_spaces() {
    while (isspace(*expr)) expr++;
}

Node* parse_primary();
Node* parse_mul();
Node* parse_expr();

Node* parse_primary() {
    skip_spaces();
    if (*expr == '(') {
        expr++;
        Node* node = parse_expr();
        skip_spaces();
        if (*expr == ')') expr++;
        return node;
    } else if (isalpha(*expr)) {
        char buf[32]; int i = 0;
        while (isalnum(*expr)) buf[i++] = *expr++;
        buf[i] = 0;
        return create_node(0, buf, NULL, NULL);
    }
    return NULL;
}

Node* parse_mul() {
    Node* left = parse_primary();
    skip_spaces();
    while (*expr == '*') {
        char op = *expr++;
        Node* right = parse_primary();
        left = create_node(op, NULL, left, right);
        skip_spaces();
    }
    return left;
}

Node* parse_expr() {
    Node* left = parse_mul();
    skip_spaces();
    while (*expr == '+') {
        char op = *expr++;
        Node* right = parse_mul();
        left = create_node(op, NULL, left, right);
        skip_spaces();
    }
    return left;
}

Node* parse(const char* input) {
    expr = input;
    return parse_expr();
}

// ======================
// Сравнение деревьев (учёт коммутативности)
// ======================
int compare(Node* a, Node* b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;

    if (a->op != b->op) return 0;
    if (a->op == 0) return strcmp(a->val, b->val) == 0;

    if (a->op == '+' || a->op == '*') {
        return (compare(a->left, b->left) && compare(a->right, b->right)) ||
               (compare(a->left, b->right) && compare(a->right, b->left));
    }

    return compare(a->left, b->left) && compare(a->right, b->right);
}

int count_matches(Node* pattern, Node* tree) {
    if (!tree) return 0;
    int count = compare(pattern, tree) ? 1 : 0;
    count += count_matches(pattern, tree->left);
    count += count_matches(pattern, tree->right);
    return count;
}

// ======================
// Тестирование
// ======================
int main() {
    char input1[256], input2[256];
    printf("Введите подвыражение (что ищем):\n> ");
    fgets(input1, sizeof(input1), stdin);
    input1[strcspn(input1, "\n")] = 0;

    printf("Введите основное выражение:\n> ");
    fgets(input2, sizeof(input2), stdin);
    input2[strcspn(input2, "\n")] = 0;

    Node* pattern = parse(input1);
    Node* full = parse(input2);

    int result = count_matches(pattern, full);
    printf("\nНайдено вхождений: %d\n", result);

    free_tree(pattern);
    free_tree(full);
    return 0;
}


