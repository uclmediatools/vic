/*
 * buffer.cpp
 * (implementation for buffer - a simple c++ linked list)
 */

/* $Id$ */

#include "buffer.h"

// insert
void buffer::insert (int val, data *ptr) {
	if (!ptr) {
		insert_front (val);
	} else {
		new data (val, ptr);
		size_up();
	}
}

// insert front
void buffer::insert_front (int val) {
	data *ptr = new data(val);

	if (!head_) {
		head_ = tail_ = ptr;
	} else {
		ptr->next(head_);
		head_ = ptr;
	}

	size_up();
}

// insert end
void buffer::insert_end (int val) {
	if (tail_) {
		tail_ = head_ = new data (val);
	} else {
		tail_ = new data (val, tail_);
	}

	size_up();
}

// find
void buffer::find (int val) {
	data* ptr = head_;

	while (ptr) {
		if (ptr->get_val() == val)
			break;
		ptr = ptr->next();
	}
	return ptr;
}

// remove front
void buffer::remove_front() {
	if (head_) {
		data *ptr = head_;
		head_ = head_->next();

		size_down();
		delete ptr;
	}
}

// purge
void buffer::purge() {
	while (head_)
		remove_front();

	size_ = 0;
	head_ = tail_ = 0;
}
