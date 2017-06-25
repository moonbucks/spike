#ifndef _RISCV_TREE_H
#define _RISCV_TREE_H

class tree
{

    struct node
    {
        uint64_t data;  // block_id
        struct node *left;
        struct node *right;
    };

    node* root;
}

#endif
