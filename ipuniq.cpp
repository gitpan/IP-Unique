/*	ipuniq.cpp - Copyright 2004
			Jay Bonci, Open Source Development Network
			jaybonci@cpan.org

	This C++ class was written to support the IP::Unique perl module
	It shares the same license, but may be used separately, if you wish.
*/

#include <string>
#include <iostream>
#include <list>
#include <math.h>
using namespace std;
#include "ipuniq.h"

#define IPADDYMAX 16
#define IPPARTMAX 4
#define IPPARTLENMAX 3
#define IPNUMMAX 255

#define CLASS_A 0
#define CLASS_B 1
#define CLASS_C 2
#define CLASS_D 3

ipuniq::ipuniq()
{
	// Clear out all of our values

	unique_ips = 0;
	total_ips = 0;
	is_sorted = 0;
	ip_part_table = 0;
	ip_numerical_table = 0;

	return;
}

ipuniq::~ipuniq()
{
	//We'll need to destry the dynamically allocated stuff

	this->clear_work_tables();
	return;
}

/* 	add_ip() - The main exposed function to handle IPs addition. This controls all of the flow
	Returns success or failure of the insertion (failure usually due to poorly formatted IPs).
*/

int ipuniq::add_ip(string ipstr)
{
	//return if we don't get a string.
	if(ipstr.length() == 0)
	{
		return 0;
	}

	this->clear_work_tables();

	if(this->split_ip_address(ipstr) != 1)
	{
		//If it doesn't appear to have four correct parts, we can't make an IP out of it.
		return 0;
	}
	
	//We need it to be formed correctly and in an acceptible range.
	if(this->is_formed_ok())
	{
		if(this->numerify_ip_address() != 1)
		{
			return 0;
		}

		return this->insert_ip();

		return 1;
	}

	return 0;
}

/* 
	split_ip_address() - Unexposed function that does all of the parsing and inserts into an internal work structure.
   	Returns success or failure of the split.
*/

bool ipuniq::split_ip_address(const string ipstr)
{
	if(ipstr.length() == 0)
	{
		return 0;
	}

	string ipstr_scrap = ipstr;

	this->new_ip_part_table();

	string::size_type current_pos = (string::size_type) 0;
	string::size_type dot_pos = ipstr_scrap.find(".", current_pos);


	//TODO: Restructure this loop

	if(dot_pos == string::npos)
	{
		//No dots at all!
		return 0;
	}

	int ip_part_index = 0;
	while(dot_pos != string::npos && ip_part_index < IPPARTMAX - 1)
	{
		ip_part_table[ip_part_index] = ipstr_scrap.substr(current_pos, dot_pos-current_pos);

		if(dot_pos == ipstr_scrap.length())
		{
			break;
		}

		current_pos = dot_pos+1;
		ip_part_index++;			

		dot_pos = ipstr_scrap.find(".", current_pos);
	}

		if(ip_part_index < IPPARTMAX)
		{
			ip_part_table[ip_part_index] = ipstr_scrap.substr(current_pos, ipstr_scrap.length()-current_pos);
		}

	return 1;

}

/*	is_formed_ok() - Unexposed function that runs a series of tests on a the work table to see if the data is the right size,
	composed entirely of numbers, etc.
	Returns true/false status of formed_ness (per rules of IPv4)
	
*/

int ipuniq::is_formed_ok(void)
{
	//No ip_part_table is a bad ip_part_table

	if(ip_part_table == NULL)
	{
		return 0;
	}

	//IP Addresses should have no fewer than four parts that are
	
	for(int ip_part_index = 0; ip_part_index < IPPARTMAX; ip_part_index++)
	{
	
		//...not null

		if(ip_part_table[ip_part_index].length() == 0)
		{
			return 0;
		}

		//...not larger than the ip class numerical max (3 characters)
		//...not zero-length

		string::size_type part_length = ip_part_table[ip_part_index].length();
		if(part_length > IPPARTLENMAX or part_length == 0)
		{
			return 0;
		}

		if(ip_part_table[ip_part_index].find_first_not_of("0123456789", 0) != string::npos)
		{
			return 0;
		}

	}

	return 1;
}

/* 	new_ip_part_table() - Unexposed function to create a new ip_part_table (an internal structure), if needed
   	Returns nothing.
*/

void ipuniq::new_ip_part_table(void)
{
	this->delete_ip_part_table(); // Really only wipes the table

	if(ip_part_table == NULL)
	{
		ip_part_table = new string[IPPARTMAX](IPPARTLENMAX,'\0');
	}

	return;
}

/*	numerify_ip_address(void) - Unexposed function that takes the internal stringified IP address and makes it an array of ints. 
	Returns true if successfully numerified. Returns false if the numbers are out of range (usually, > 255)
*/

bool ipuniq::numerify_ip_address(void)
{

	this->new_ip_numerical_table();

	for(int ip_part_index = 0; ip_part_index < IPPARTMAX; ip_part_index++)
	{
		ip_numerical_table[ip_part_index] = atoi(ip_part_table[ip_part_index].c_str());

		if(ip_numerical_table[ip_part_index] > IPNUMMAX)
		{
			this->delete_ip_numerical_table();
			return 0;
		}
	}

	return 1;	
}

/*	insert_ip() - Unexposed function that takes a numerified IP address, and inserts it into the STL list
	Fails if we don't have a valid numerified IP address. Returns true otherwise
*/

bool ipuniq::insert_ip(void)
{
	if(ip_numerical_table == 0)
	{
		return 0;
	}

	unsigned int bitnum = 0;

	for(int i = 0; i<IPPARTMAX; i++)
	{
		bitnum = bitnum << 8;
		bitnum += ip_numerical_table[i];
	}

	is_sorted = 0;
	ip_table.push_front(bitnum);
	total_ips++; //We can't touch this number;	

	return 1;		
}

/*	compact() - Exposed function that de-dupes the internal IP address list and refreshes the unique() count
	Returns nothing
*/

void ipuniq::compact(void)
{

	ip_table.sort();

	list<unsigned int>::iterator list_pos = ip_table.begin();
	list<unsigned int>::iterator next_pos = list_pos;
	unsigned int list_size = ip_table.size();
	unsigned int list_spot = 1;

	unique_ips = 0;
	is_sorted = 1;

	if(list_size == 0)
	{
		return;
	}

	while(list_spot <= list_size)
	{
		next_pos++;

		if(*(next_pos) < *(list_pos) || list_spot == list_size) //We've wrapped around
		{
			unique_ips++;
			return;
		}

		if(*(list_pos) == *(next_pos))
		{
			ip_table.erase(list_pos);
		}else{
			unique_ips++;
		}
		
		list_pos = next_pos;
		list_spot++;
	}	

	is_sorted = 1;

	return;
}

/*	unique() - Exposed function that returns the number of unique IPv4 addresses in the list as an int. Can take some time to return
*/

unsigned int ipuniq::unique(void)
{
	if(!is_sorted)
	{
		this->compact();
	}
	return unique_ips;
}

/*	total() - Exposed function that returns the total number of addresses in the list as an int.
*/

unsigned int ipuniq::total(void)
{
	return total_ips;
}

/*	delete_ip_part_table() - Unexposed function to clean up a temporary work table
*/

void ipuniq::delete_ip_part_table(void)
{
	if(ip_part_table == NULL)
	{
		return;
	}

	for(int i=0; i < IPPARTMAX; i++)
	{
		ip_part_table[i] = "";
	}

	//delete ip_part_table;
	//ip_part_table = 0;
	return;
}

/*	delete_ip_numerical_table() - Unexposed function to clean up a temporary work table
*/

void ipuniq::delete_ip_numerical_table(void)
{
	if(ip_numerical_table == NULL)
	{
		return;
	}

	delete ip_numerical_table;
	ip_numerical_table = 0;
	return;
}

/*	clear_work_tables() - Unexposed function to summerize work table cleanum
*/

void ipuniq::clear_work_tables(void)
{
	this->delete_ip_part_table();
	this->delete_ip_numerical_table();

	return;
}

/*	new_ip_numerical_table() - Unexposed function to build an internal work structure.
*/

void ipuniq::new_ip_numerical_table(void)
{
	this->delete_ip_numerical_table();
	ip_numerical_table = new int[IPPARTMAX];
	
	return;
}
