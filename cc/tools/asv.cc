/*
 * Copyright(c) 2010 University College London
 * All rights reserved.
 *
 * AUTHOR: Soo-Hyun Choi <s.choi@cs.ucl.ac.uk>
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE
 *
 * $Id$
 */

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

int main (int argc, char *argv[]) {

	if (argc < 2) {
		cout << "Usage: ./asv <desired output filename> <input file>" << endl;
		exit (0);
	}

	ifstream fin (argv[1]);
	string option = argv[2]; 
	ofstream fout;

	// variables
	string items, stat;
	double time;
	int seqno;

	if (fin.is_open()) {
		// preparing for the output file
		stringstream ss;
		ss << option << ".xg";
		fout.open(ss.str().c_str());

		// get input file stream
		while (getline(fin, items)) {
			istringstream is(items);
			is >> time >> seqno;
			fout << time << "\t" << (seqno%100) << endl;
		} // while

		fin.close();
		fout.close();
	} else {
			cout << "Unable to open file!: " << argv[1] << endl;
	}

	return 0;
}
