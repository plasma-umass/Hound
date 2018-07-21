#ifndef HOUND_BLOCK_LIST_H
#define HOUND_BLOCK_LIST_H

#include <cassert>

class AOCommon;

template <class T>
class BlockListImpl {
public:
  class Node {
  public:
    inline Node *&next() {
      return _next;
    }
    inline Node *&prev() {
      return _prev;
    }
    inline BlockListImpl<T> *&list() {
      return _list;
    }
    inline BlockListImpl<T> *const &list() const {
      return _list;
    }

    T *const data;

    Node(T *d) : data(d), _list(NULL), _next(NULL), _prev(NULL) {
    }

  protected:
    // Needs to be accessible from subclasses of BlockListImpl
    BlockListImpl<T> *_list;

    Node *_prev;
    Node *_next;
  };

  virtual void registerBlock(Node *bl) {
    assert(!bl->list());
    // XXX PEDANTIC
    // assert(!reachable(bl));

    bl->next() = head;
    bl->prev() = NULL;

    if (head)
      head->prev() = bl;
    else {
      // empty
      assert(_ct == 0);
      tail = bl;
    }

    head = bl;

    bl->list() = this;

    // fprintf(stderr,"(%p) added block %p\n",this,bl);

    assert(reachable(bl));

    _ct++;
  }

  virtual void removeBlock(Node *bl) {
    assert(bl->list() == this);
    // XXX PEDANTIC
    // assert(reachable(bl));

    if (bl == head) {
      head = head->next();
      if (head)
        head->prev() = NULL;
    } else {
      bl->prev()->next() = bl->next();
      if (bl->next()) {
        bl->next()->prev() = bl->prev();
      }
    }

    if (bl == tail)
      tail = tail->prev();

    bl->prev() = bl->next() = NULL;

    bl->list() = NULL;

    _ct--;

    // XXX PEDANTIC
    // assert(!reachable(bl));
  }

  size_t getSize() const {
    return _ct;
  }

  /*
  // optional
  virtual void barrierBlock(ListNode<F> * bl) {
    //assert(!bl->isProtected());
    // Can be called from MemTracer stuff (syscalls), so Active list must handle
    //fprintf(stderr,"invalid barrier... not expecting!\n");
    //abort();
  }
  */

  Node *getHead() const {
    return head;
  }

protected:
  BlockListImpl() : head(NULL), tail(NULL), _ct(0) {
  }

  bool reachable(Node *bl) {
    for (Node *curr = head; curr != NULL; curr = curr->next()) {
      assert(curr->list() == this);
      if (bl == curr)
        return true;
    }

    return false;
  }

  Node *head;
  Node *tail;

  size_t _ct;
};
#endif
