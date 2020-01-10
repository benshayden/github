template<class T>
class List {
 private:
  class ListNode {
   public:
    T value;
    ListNode* next;
    ListNode(T v, ListNode* n) : value(v), next(n) {}
  };
  uint32_t length_;
  ListNode* head_;

 public:
  List() : length_(0), head_(nullptr) {}

  void clear() {
    while (length_) {
      pop();
    }
  }

  void prepend(T element) {
    ++length_;
    head_ = new ListNode(element, head_);
  }

  void append(T element) {
    if (length_ == 0) {
      prepend(element);
      return;
    }
    ++length_;
    ListNode* node = head_;
    while (node->next) node = node->next;
    node->next = new ListNode(element, nullptr);
  }

  uint32_t length() { return length_; }

  T get(uint32_t index) {
    ListNode* node = head_;
    while (index) {
      --index;
      node = node->next;
    }
    return node->value;
  }

  T pop() {
    ListNode* node = head_;
    T value = node->value;
    head_ = node->next;
    delete node;
    --length_;
    return value;
  }

  void sort() {
    if (length_ < 2) return;
    for (uint32_t pass = 0; pass < length_ - 1; ++pass) {
      bool sorted = true;
      ListNode* current = head_;
      for (uint32_t index = 0; index < length_ - pass - 1; ++index) {
        if (current->value > current->next->value) {
          sorted = false;
          T temp = current->value;
          current->value = current->next->value;
          current->next->value = temp;
        }
        current = current->next;
      }
      if (sorted) break;
    }
  }
};
