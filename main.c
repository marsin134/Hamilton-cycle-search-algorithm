#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <limits.h>

#define MAX_VERTICES 20
#define MAX_PRINT_CYCLES 100

typedef struct {
    int n;
    int directed;
    int adj[MAX_VERTICES][MAX_VERTICES];
} Graph;

typedef struct {
    const Graph *graph;
    int path[MAX_VERTICES + 1];
    int used[MAX_VERTICES];
    uint64_t count;
    uint64_t recursive_calls;
    int print_cycles;
    int printed;
    int stop_after_first;
    int found_first;
} SearchState;

static void clear_input_line(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) { }
}

static int read_int(const char *prompt, int min_value, int max_value) {
    int value;
    for (;;) {
        printf("%s", prompt);
        if (scanf("%d", &value) == 1) {
            clear_input_line();
            if (value >= min_value && value <= max_value)
                return value;
        } else {
            clear_input_line();
        }
        printf("Некорректный ввод. Допустимый диапазон: %d..%d.\n",
               min_value, max_value);
    }
}

static void init_graph(Graph *g, int n, int directed) {
    int i, j;
    g->n = n;
    g->directed = directed;
    for (i = 0; i < MAX_VERTICES; ++i)
        for (j = 0; j < MAX_VERTICES; ++j)
            g->adj[i][j] = 0;
}

static void print_graph(const Graph *g) {
    int i, j;
    printf("\nТип графа: %s\n", g->directed ? "ориентированный" : "неориентированный");
    printf("Количество вершин: %d\n", g->n);
    printf("Матрица смежности:\n    ");
    for (j = 0; j < g->n; ++j) printf("%3d", j + 1);
    putchar('\n');
    for (i = 0; i < g->n; ++i) {
        printf("%3d ", i + 1);
        for (j = 0; j < g->n; ++j) printf("%3d", g->adj[i][j]);
        putchar('\n');
    }
}

static int load_graph(const char *filename, Graph *g) {
    FILE *file = fopen(filename, "r");
    int n, directed, i, j, value;

    if (!file) {
        printf("Не удалось открыть файл \"%s\".\n", filename);
        return 0;
    }

    if (fscanf(file, "%d%d", &directed, &n) != 2 ||
        (directed != 0 && directed != 1) ||
        n < 3 || n > MAX_VERTICES) {
        printf("Ошибка формата файла. Ожидаются: directed(0/1), n(3..%d).\n",
               MAX_VERTICES);
        fclose(file);
        return 0;
    }

    init_graph(g, n, directed);
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            if (fscanf(file, "%d", &value) != 1 || (value != 0 && value != 1)) {
                printf("Ошибка чтения матрицы смежности.\n");
                fclose(file);
                return 0;
            }
            g->adj[i][j] = value;
        }
    }
    fclose(file);

    if (!directed) {
        for (i = 0; i < n; ++i) {
            for (j = i + 1; j < n; ++j) {
                if (g->adj[i][j] != g->adj[j][i]) {
                    printf("Ошибка: матрица неориентированного графа должна быть симметричной.\n");
                    return 0;
                }
            }
        }
    }
    for (i = 0; i < n; ++i) g->adj[i][i] = 0;
    return 1;
}

static void backtrack(SearchState *s, int depth) {
    const Graph *g = s->graph;
    int next;

    if (s->stop_after_first && s->found_first) return;
    s->recursive_calls++;

    if (depth == g->n) {
        int start = s->path[0];
        int last = s->path[g->n - 1];

        if (g->adj[last][start] && last != start) {
            if (!g->directed && s->path[1] > s->path[g->n - 1])
                return;

            s->count++;
            s->found_first = 1;

            if (s->print_cycles && s->printed < MAX_PRINT_CYCLES) {
                int i;
                printf("Цикл %llu: ", (unsigned long long)s->count);
                for (i = 0; i < g->n; ++i)
                    printf("%d -> ", s->path[i] + 1);
                printf("%d\n", start + 1);
                s->printed++;
            }
        }
        return;
    }

    for (next = 1; next < g->n; ++next) {
        if (!s->used[next] && g->adj[s->path[depth - 1]][next]) {
            s->path[depth] = next;
            s->used[next] = 1;
            backtrack(s, depth + 1);
            s->used[next] = 0;
            if (s->stop_after_first && s->found_first) return;
        }
    }

}

static void find_hamiltonian_cycles(const Graph *g, int print_cycles,
                                    int stop_after_first) {
    SearchState state;
    clock_t begin, end;
    double elapsed;

    memset(&state, 0, sizeof(state));
    state.graph = g;
    state.print_cycles = print_cycles;
    state.stop_after_first = stop_after_first;
    state.path[0] = 0;
    state.used[0] = 1;

    printf("\nПоиск гамильтоновых циклов...\n");
    begin = clock();
    backtrack(&state, 1);
    end = clock();

    elapsed = (double)(end - begin) / CLOCKS_PER_SEC;
    if (state.count == 0) {
        printf("Гамильтоновы циклы не найдены.\n");
    } else if (stop_after_first) {
        printf("Найден как минимум один гамильтонов цикл.\n");
    } else {
        printf("Всего найдено гамильтоновых циклов: %llu\n",
               (unsigned long long)state.count);
    }

    if (print_cycles && state.count > MAX_PRINT_CYCLES)
        printf("Выведены первые %d циклов; остальные учтены в общем количестве.\n",
               MAX_PRINT_CYCLES);
    printf("Рекурсивных вызовов: %llu\n",
           (unsigned long long)state.recursive_calls);
    printf("Время выполнения: %.6f сек.\n", elapsed);
}

static void create_manual_graph(Graph *g) {
    int n = read_int("Количество вершин (3..20): ", 3, MAX_VERTICES);
    int directed = read_int("Тип графа (0 - неориентированный, 1 - ориентированный): ", 0, 1);
    int i, j, value;

    init_graph(g, n, directed);
    printf("Введите матрицу смежности %d x %d (только 0 или 1).\n", n, n);
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            char prompt[80];
            snprintf(prompt, sizeof(prompt), "A[%d][%d] = ", i + 1, j + 1);
            value = read_int(prompt, 0, 1);
            g->adj[i][j] = value;
        }
    }
    for (i = 0; i < n; ++i) g->adj[i][i] = 0;

    if (!directed) {
        for (i = 0; i < n; ++i) {
            for (j = i + 1; j < n; ++j) {
                if (g->adj[i][j] != g->adj[j][i]) {
                    printf("Матрица была приведена к симметричному виду.\n");
                    g->adj[j][i] = g->adj[i][j];
                }
            }
        }
    }
    printf("Граф успешно создан.\n");
}

static void generate_random_graph(Graph *g) {
    int n = read_int("Количество вершин (3..20): ", 3, MAX_VERTICES);
    int directed = read_int("Тип графа (0 - неориентированный, 1 - ориентированный): ", 0, 1);
    int percent = read_int("Вероятность ребра в процентах (0..100): ", 0, 100);
    int i, j;

    init_graph(g, n, directed);
    for (i = 0; i < n; ++i) {
        for (j = (directed ? 0 : i + 1); j < n; ++j) {
            if (i == j) continue;
            g->adj[i][j] = (rand() % 100 < percent) ? 1 : 0;
            if (!directed) g->adj[j][i] = g->adj[i][j];
        }
    }
    printf("Случайный граф сгенерирован.\n");
}

static int run_tests(void) {
    Graph g;
    int passed = 0;
    SearchState s;

    printf("\n=== Автоматические тесты ===\n");

    load_graph("tests/test1.txt", &g);
    memset(&s, 0, sizeof(s)); s.graph = &g; s.path[0] = 0; s.used[0] = 1;
    backtrack(&s, 1);
    printf("Тест 1 (треугольник, ожидается 1): %s, получено %llu\n",
           s.count == 1 ? "ПРОЙДЕН" : "НЕ ПРОЙДЕН",
           (unsigned long long)s.count);
    if (s.count == 1) passed++;

    load_graph("tests/test2.txt", &g);
    memset(&s, 0, sizeof(s)); s.graph = &g; s.path[0] = 0; s.used[0] = 1;
    backtrack(&s, 1);
    printf("Тест 2 (полный граф K4, ожидается 3): %s, получено %llu\n",
           s.count == 3 ? "ПРОЙДЕН" : "НЕ ПРОЙДЕН",
           (unsigned long long)s.count);
    if (s.count == 3) passed++;

    load_graph("tests/test3.txt", &g);
    memset(&s, 0, sizeof(s)); s.graph = &g; s.path[0] = 0; s.used[0] = 1;
    backtrack(&s, 1);
    printf("Тест 3 (ориентированный цикл, ожидается 1): %s, получено %llu\n",
           s.count == 1 ? "ПРОЙДЕН" : "НЕ ПРОЙДЕН",
           (unsigned long long)s.count);
    if (s.count == 1) passed++;

    load_graph("tests/test4.txt", &g);
    memset(&s, 0, sizeof(s)); s.graph = &g; s.path[0] = 0; s.used[0] = 1;
    backtrack(&s, 1);
    printf("Тест 4 (цепочка без цикла, ожидается 0): %s, получено %llu\n",
           s.count == 0 ? "ПРОЙДЕН" : "НЕ ПРОЙДЕН",
           (unsigned long long)s.count);
    if (s.count == 0) passed++;

    printf("Итог: %d из 4 тестов пройдено.\n", passed);
    return passed == 4;
}

static void show_menu(void) {
    printf("\n===============================================\n");
    printf("   ПОИСК ГАМИЛЬТОНОВЫХ ЦИКЛОВ В ГРАФЕ\n");
    printf("===============================================\n");
    printf("1. Пример реализации (демонстрационный граф)\n");
    printf("2. Найти все циклы в текущем графе\n");
    printf("3. Проверить существование гамильтонова цикла\n");
    printf("4. Сгенерировать случайный граф\n");
    printf("5. Загрузить граф из файла\n");
    printf("6. Ввести граф вручную\n");
    printf("7. Запустить автоматические тесты\n");
    printf("8. Показать текущий граф\n");
    printf("0. Выход\n");
    printf("-----------------------------------------------\n");
}

int main(void) {
    Graph graph;
    int has_graph = 0;
    int choice;
    char filename[256];

    srand((unsigned int)time(NULL));

    for (;;) {
        show_menu();
        choice = read_int("Выберите пункт меню: ", 0, 10);

        switch (choice) {
            case 1:
                if (load_graph("exampleGraph.txt", &graph)) {
                    has_graph = 1;
                    printf("Загружен демонстрационный граф.\n");
                    print_graph(&graph);
                    find_hamiltonian_cycles(&graph, 1, 0);
                }
                break;
            case 2:
                if (!has_graph) printf("Сначала создайте или загрузите граф.\n");
                else find_hamiltonian_cycles(&graph, 1, 0);
                break;
            case 3:
                if (!has_graph) printf("Сначала создайте или загрузите граф.\n");
                else find_hamiltonian_cycles(&graph, 0, 1);
                break;
            case 4:
                generate_random_graph(&graph);
                has_graph = 1;
                print_graph(&graph);
                break;
            case 5:
                printf("Имя файла (например, graphs.txt): ");
                if (scanf("%255s", filename) == 1) {
                    clear_input_line();
                    if (load_graph(filename, &graph)) {
                        has_graph = 1;
                        printf("Граф успешно загружен.\n");
                        print_graph(&graph);
                    }
                } else clear_input_line();
                break;
            case 6:
                create_manual_graph(&graph);
                has_graph = 1;
                print_graph(&graph);
                break;
            case 7:
                run_tests();
                break;
            case 8:
                if (has_graph) print_graph(&graph);
                else printf("Граф ещё не создан.\n");
                break;
            case 0:
                printf("Завершение программы.\n");
                return 0;
        }
    }
}
