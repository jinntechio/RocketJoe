#include <cstdlib>
#include <exception>
#include <locale>

#include <boost/filesystem.hpp>
#include <boost/locale/generator.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "configuration.hpp"
#include <components/configuration/configuration.hpp>
#include <components/log/log.hpp>
#include <components/process_pool/process_pool.hpp>
#include <components/python/python.hpp>
#include <services/jupyter/jupyter.hpp>

#include "init_service.hpp"

#ifdef __APPLE__

#else

#include <boost/stacktrace.hpp>
#include <csignal>

void my_signal_handler(int signum) {
    ::signal(signum, SIG_DFL);
    std::cerr << "signal called:"
              << std::endl
              << boost::stacktrace::stacktrace()
              << std::endl;
    ::raise(SIGABRT);
}

void setup_handlers() {
    ::signal(SIGSEGV, &my_signal_handler);
    ::signal(SIGABRT, &my_signal_handler);
}

static auto original_terminate_handler{std::get_terminate()};

void terminate_handler() {
    std::cerr << "terminate called:"
              << std::endl
              << boost::stacktrace::stacktrace()
              << std::endl;
    original_terminate_handler();
    std::abort();
}

#endif

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
#ifdef __APPLE__

#else
    setup_handlers();

    std::set_terminate(terminate_handler);
#endif

    auto log = components::initialization_logger();

    std::locale::global(boost::locale::generator{}(""));

    std::vector<std::string> all_args(argv, argv + argc);

    po::options_description command_line_description("Allowed options");
    // clang-format off
    command_line_description.add_options()
        ("help", "Print help")
        ("worker_mode", "Worker Mode")
        ("worker_number",po::value<unsigned short int>(), "Worker Process Mode and run  ")
        ("jupyter_connection", po::value<boost::filesystem::path>(),"path to jupyter connection file")
        ("ssh", "Connect to ipcontroller via SSH")
        ("ssh_host", po::value<std::string>(), "ipcontroller host")
        ("ssh_port", po::value<std::uint16_t>(), "ipcontroller port")
        ;
    // clang-format on
    po::variables_map command_line;

    po::store(
        po::command_line_parser(all_args)
            .options(command_line_description)
            .allow_unregistered() /// todo hack
            .run(),
        command_line);

    log.info(logo());
    log.info(fmt::format("Command  Args : {0}", fmt::join(all_args, " , ")));

    /// --help option
    if (command_line.count("help")) {
        std::cerr << command_line_description << std::endl;
        return 0;
    }

    components::configuration cfg_;
    cfg_.python_configuration_.mode_= components::sandbox_mode_t::jupyter_engine;
    cfg_.operating_mode_ = components::operating_mode::master;

    if (command_line.count("jupyter_connection")) {
        cfg_.python_configuration_.jupyter_connection_path_ = command_line["jupyter_connection"].as<boost::filesystem::path>();
    }

    if (command_line.count("ssh")) {
        cfg_.python_configuration_.ssh_ = true;
    }

    if (command_line.count("ssh_host")) {
        cfg_.python_configuration_.ssh_host_ = command_line["ssh_host"].as<std::string>();
    }

    if (command_line.count("ssh_port")) {
        cfg_.python_configuration_.ssh_port_ = command_line["ssh_port"].as<std::uint16_t>();
    }

    if (command_line.count("worker_mode")) {
        log.info("Worker Mode");
        cfg_.operating_mode_ = components::operating_mode::worker;
    }

    components::process_pool_t process_pool(all_args[0], {"--worker_mode"}, log);

    if (command_line.count("worker_number")) {
        process_pool.add_worker_process(command_line["worker_number"].as<std::size_t>()); /// todo hack
    }

    auto jupyter = services::make_manager_service<services::jupyter>(cfg_,log);
    init_service(jupyter, cfg_, log);
    jupyter->start();

    log.info("Shutdown RocketJoe");
    return 0;
}
