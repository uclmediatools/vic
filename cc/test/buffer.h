/*
 * buffer.h
 * (pktbuf emulation for unit test)
 */

/* $Id$ */

#include "config.h"

// buffer is a linked list
class data;

class buffer {
public:
	// default constructor
	buffer() : head_(0), tail_(0), size_(0) {};

	// return list size
	inline int size() { return size_; }

	// increase list size
	inline void size_up() { ++size_; }

	// decrease list size
	inline void size_down() { --size_; }

	// insert
	void insert (int val, data *ptr);

	// insert front
	void insert_front (int val);

	// insert end
	void insert_end (int val);

	// find
	data* find (int val);

	// remove
	int remove (int val);

	// remove front
	void remove_front();

	// purge list
	void purge();

private:
	data* head_;
	data* tail_;
	int size_;
};

// buffer contents 
class data {
public:
	friend class buffer;

	// default constructor
	data() : val_(0), next_(0) {};
	data (int val, data *loc = 0);

	// value
	int get_val() { return val_; }

	// next
	data* next() { return next_; }

private:
	int val_;		// hold a number 
	data *next_;	// hold the address of the next item on the list
};
