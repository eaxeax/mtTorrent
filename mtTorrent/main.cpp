#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <RiotRestApi.h>
#include "Communicator.h"


int main(int argc, char* argv[])
{
	try
	{
		//riotApiRequest();

		Torrent::Communicator torrent;
		torrent.test();
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}

	std::cout << "Count: " << std::to_string(gcount) << "\n";

	return 0;
}