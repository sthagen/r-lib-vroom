#pragma once

#include "iterator.h"
#include "vroom.h"

#include <memory>
#include <vector>

namespace vroom {
class index {

public:
  class subset_iterator : public base_iterator {
    size_t i_;
    iterator it_;
    std::shared_ptr<std::vector<size_t> > indexes_;

  public:
    subset_iterator(
        const iterator& it,
        const std::shared_ptr<std::vector<size_t> >& indexes)
        : i_(0), it_(it), indexes_(indexes) {}
    void next() { ++i_; }
    void prev() { --i_; }
    void advance(ptrdiff_t n) { i_ += n; }
    bool equal_to(const base_iterator& other) const {
      auto other_ = static_cast<const subset_iterator*>(&other);
      return i_ == other_->i_;
    };
    ptrdiff_t distance_to(const base_iterator& that) const {
      auto that_ = static_cast<const subset_iterator*>(&that);
      return that_->i_ - i_;
    };
    string value() const { return *(it_ + (*indexes_)[i_]); };
    subset_iterator* clone() const {
      auto copy = new subset_iterator(*this);
      return copy;
    };

    string at(ptrdiff_t n) const { return it_[(*indexes_)[n]]; }

    virtual ~subset_iterator() {}
  };

  class range {
    const iterator begin_;
    const iterator end_;

  public:
    range(const iterator& begin, const iterator& end)
        : begin_(begin), end_(end) {}
    range(base_iterator* begin, base_iterator* end)
        : begin_(begin), end_(end) {}
    iterator begin() { return begin_; }
    iterator end() { return end_; }
    size_t size() const { return end_ - begin_; }
    string at(size_t i) const { return begin_[i]; }
    std::shared_ptr<vroom::index::range>
    subset(const std::shared_ptr<std::vector<size_t> >& idx) const {
      auto begin = new subset_iterator(begin_, idx);
      auto end = new subset_iterator(begin_, idx);
      end->advance(idx->size());
      return std::make_shared<vroom::index::range>(begin, end);
    }
    std::shared_ptr<vroom::index::range> slice(size_t start, size_t end) const {
      return std::make_shared<vroom::index::range>(
          begin_ + start, begin_ + end);
    }
  };

  using column = range;
  using row = range;

  virtual std::shared_ptr<row> get_row(size_t row) const = 0;
  virtual std::shared_ptr<row> get_header() const = 0;

  virtual std::shared_ptr<column> get_column(size_t col) const = 0;

  virtual size_t num_columns() const = 0;
  virtual size_t num_rows() const = 0;

  virtual string get(size_t row, size_t col) const = 0;
  virtual std::string get_delim() const = 0;
  virtual ~index() {}
};

} // namespace vroom
