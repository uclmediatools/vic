/*
 * buffer.h
 * (pktbuf emulation for unit test)
 */

/* $Id$ */

#ifndef vic_cc_test_buffer_h
#define vic_cc_test_buffer_h

#include <iostream>
#include "config.h"

// Buffer is a linked list
class Data;

class Buffer {
public:
	// default constructor
	Buffer() : head_(NULL), tail_(NULL), size_(0) {};

	// destructor
	~Buffer() {};

	// return list size
	inline int size() { return size_; }

	// increase list size
	inline void size_up() { ++size_; }

	// decrease list size
	inline void size_down() { --size_; }

	// insert
	void insert (int val, Data *ptr);

	// insert front
	void insert_front (int val);

	// insert end
	void insert_end (int val);

	// find
	Data* find (int val);

	// remove
	int remove (int val);

	// remove front
	void remove_front();

	// purge list
	void purge();

	// display
	void display (std::ostream &out = std::cout);

private:
	Data* head_;
	Data* tail_;
	int size_;
};

// Buffer contents 
class Data {
public:
	// default constructor
	Data() : val_(0), next_(NULL) {};
	Data (int val, Data *ptr = NULL);

	// destructor
	~Data() {};

	// value
	int get_val() { return val_; }

	// next
	Data* next() { return next_; }

	// set to next
	void next (Data *ptr) { next_ = ptr; }

private:
	int val_;		// hold a number 
	Data *next_;	// hold the address of the next item on the list
};

#endif /* vic_cc_test_buffer_h */
