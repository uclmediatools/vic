/*
 * buffer.cpp
 * (implementation for Buffer - a simple c++ linked list)
 */

/* $Id$ */

#include "buffer.h"

// insert
void Buffer::insert (int val, Data *ptr) {
	if (!ptr) {
		insert_front (val);
	} else {
		new Data (val, ptr);
		size_up();
	}
}

// insert front
void Buffer::insert_front (int val) {
	Data *ptr = new Data(val);

	if (!head_) {
		head_ = tail_ = ptr;
	} else {
		ptr->next(head_);
		head_ = ptr;
	}

	size_up();
}

// insert end
void Buffer::insert_end (int val) {
	if (tail_) {
		tail_ = head_ = new Data (val);
	} else {
		tail_ = new Data (val, tail_);
	}

	size_up();
}

// find
Data* Buffer::find (int val) {
	Data* ptr = head_;

	while (ptr) {
		if (ptr->get_val() == val)
			break;
		ptr = ptr->next();
	}
	return ptr;
}

// remove front
void Buffer::remove_front() {
	if (head_) {
		Data *ptr = head_;
		head_ = head_->next();

		size_down();
		delete ptr;
	}
}

// purge
void Buffer::purge() {
	while (head_)
		remove_front();

	size_ = 0;
	head_ = tail_ = 0;
}

// display
void Buffer::display (std::ostream &out) {
	out << "\n( " << size_ << " ) ( ";

	Data *ptr = head_;
	while (ptr) {
		out << ptr->get_val() << " ";
		ptr = ptr->next();
	}

	out << ")\n\n";
}

// Data constructor
Data::Data (int val, Data *ptr) : val_(val) {
	if (!ptr) {
		next_ = NULL;
	} else {
		next_ = ptr->next_;
		ptr->next_ = this;
	}
}
