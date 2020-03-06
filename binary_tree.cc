#include <iostream>
#include <cassert>
#include "legion.h"

using namespace Legion;

enum {
    TOP_LEVEL_TASK_ID,
    TRAVERSE_TASK_ID,
    PRINT_KEY_TASK_ID,
};

template<typename T>
struct Node {
    T key;
    Node<T> *left;
    Node<T> *right;

    Node(T key_): key(key_), left(nullptr), right(nullptr) {}
    Node(T key_, Node *left_, Node *right_): key(key_), left(left_), right(right_) {}
    void setLeft(Node *left_) { left = left_; }
    void setRight(Node *right_) { right = right_; }
};

Node<char> *example_tree()
{
    auto *o = new Node('o');
    auto *n = new Node('n');
    auto *m = new Node('m');
    auto *l = new Node('l');
    auto *k = new Node('k');
    auto *j = new Node('j');
    auto *i = new Node('i');
    auto *h = new Node('h');
    auto *g = new Node('g', n, o);
    auto *f = new Node('f', l, m);
    auto *e = new Node('e', j, k);
    auto *d = new Node('d', h, i);
    auto *c = new Node('c', f, g);
    auto *b = new Node('b', d, e);
    auto *a = new Node('a', b, c);
    return a;
}

void top_level_task(const Task *task,
                    const std::vector<PhysicalRegion> &regions,
                    Context ctx, Runtime *runtime)
{
    Node<char> *tree = example_tree();

    TaskLauncher launcher(TRAVERSE_TASK_ID, TaskArgument(tree, sizeof(Node<char>)));
    Future res = runtime->execute_task(ctx, launcher);
    res.get_result<void>();
    std::cout << std::endl;
}

void traverse_task(const Task *task,
                   const std::vector<PhysicalRegion> &regions,
                   Context ctx, Runtime *runtime)
{
    assert(task->arglen == sizeof(Node<char>));
    Node<char> *node = static_cast<Node<char> *>(task->args);
    
    {
        TaskLauncher launcher(PRINT_KEY_TASK_ID, TaskArgument(&node->key, sizeof(char)));
        runtime->execute_task(ctx, launcher);
    }
    if (node->left != nullptr) {
        TaskLauncher launcher(TRAVERSE_TASK_ID, TaskArgument(node->left, sizeof(Node<char>)));
        runtime->execute_task(ctx, launcher);
    }
    if (node->right != nullptr) {
        TaskLauncher launcher(TRAVERSE_TASK_ID, TaskArgument(node->right, sizeof(Node<char>)));
        runtime->execute_task(ctx, launcher);
    }
}

void print_key_task(const Task *task,
                    const std::vector<PhysicalRegion> &regions,
                    Context ctx, Runtime *runtime)
{
    assert(task->arglen == sizeof(char));
    char *key = static_cast<char *>(task->args);
    std::cout << *key;
}

int main(int argc, char **argv)
{
    Runtime::set_top_level_task_id(TOP_LEVEL_TASK_ID);
    {
        TaskVariantRegistrar registrar(TOP_LEVEL_TASK_ID, "top_level");
        registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
        Runtime::preregister_task_variant<top_level_task>(registrar, "top_level");
    }
    {
        TaskVariantRegistrar registrar(TRAVERSE_TASK_ID, "traverse");
        registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
        Runtime::preregister_task_variant<traverse_task>(registrar, "traverse");
    }
    {
        TaskVariantRegistrar registrar(PRINT_KEY_TASK_ID, "print key");
        registrar.add_constraint(ProcessorConstraint(Processor::LOC_PROC));
        registrar.set_leaf(true);
        Runtime::preregister_task_variant<print_key_task>(registrar, "print key");
    }

    return Runtime::start(argc, argv);
}
