/************************************************************
makefile:
g++ -c option.cpp
g++ -o a.out option.o -L/usr/local/lib -lboost_program_options

results:
run:
	./a.out --apples 20 --orangs 30
	./a.out -a 20 -o 30 --name eric
	./a.out --help

If using parse config file
	## comment: apples_oranges.cfg
	name=eric
	apples=10
	oranges=20

	./a.out
************************************************************/

#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp> // 'reading_file' exception class is declared in errors.hpp
#include <iostream>

namespace opt = boost::program_options;

int main(int argc, char *argv[]){
    opt::options_description desc("All options");
    desc.add_options()
        ("apples,a", opt::value<int>()->default_value(10), "apples that you have")
        ("oranges,o", opt::value<int>(), "oranges that you have")
        ("name", opt::value<std::string>(), "your name")
        ("help", "produce help message")
    ;

    opt::variables_map vm;
 // Parsing command line options and storing values to 'vm'
	opt::store(opt::parse_command_line(argc, argv, desc), vm);
    // We can also parse environment variables using 'parse_environment' method

// void notify(const boost::any & value_store) const;
// Called when final value of an option is determined.
    opt::notify(vm);

    if(vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    // Adding missing options from "aples_oranges.cfg" config file.
    // You can also provide an istreamable object as a first parameter for 'parse_config_file'
    // 'char' template parameter will be passed to underlying std::basic_istream object
    try {
      opt::store(opt::parse_config_file<char>("apples_oranges.cfg", desc), vm);
    } catch (const opt::reading_file& e){
        std::cout << "Failed to open file 'apples_oranges.cfg': " << e.what();
    }
    opt::notify(vm);
    if (vm.count("name")){
      std::cout << "Hi," << vm["name"].as<std::string>() << "!\n";
    }
    std::cout << "Fruits count: " << vm["apples"].as<int>() + vm["oranges"].as<int>() << std::endl;
    return 0;
}