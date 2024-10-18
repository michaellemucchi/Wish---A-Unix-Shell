#include <iostream>
#include <fstream>
#include <sstream>   // for parsing data
#include <fcntl.h>   // for open()
#include <unistd.h>  // for write(), close(), read()
#include <cstring>
#include <vector>
#include <sys/wait.h>

using namespace std;

// function declarations
void print_error();
int find_mode(int& argc);
bool handle_builtin_commands(const vector<string>& tokens, vector<string>& paths);
string search_executable_path(const string& command, vector<string>& paths);
bool parse_command(string& command, vector<vector<string>>& token_matrix, 
                   vector<bool>& file_red_vec, vector<string>& out_file_vec);
void execute_instruction(vector<string>& tokens, const string& output_file, 
                         const bool& file_redirect, vector<string>& paths);
void parse_line(string& curr_line, vector<string>& paths);


void print_error() { // copied from the prompt
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int find_mode(int& argc) { // function to find the mode of our shell
    if (argc == 1) {
        return 0; // interactive mode
    } else if (argc == 2) {
        return 1; // batch mode
    } else { // > args
        print_error();
        exit(1);
    }
}

// helper function to handle built in commmands
bool handle_builtin_commands(const vector<string>& tokens, vector<string>& paths) {
    if (tokens[0] == "exit") { // exit
        if (tokens.size() > 1) {
            print_error();
        }
        exit(0); // exit shell
    } else if (tokens[0] == "cd") { // cd
        if (tokens.size() != 2 || chdir(tokens[1].c_str()) == -1) {
            print_error();
        }
        return true;
    } else if (tokens[0] == "path") { // path
        paths.clear();
        for (size_t i = 1; i < tokens.size(); ++i) {
            paths.push_back(tokens[i]);
        }
        return true;
    }

    return false;  // not a built-in command
}

// helper function to check for possible executable path
string search_executable_path(const string& command, vector<string>& paths) {
    for (unsigned int i = 0; i < paths.size(); i++) {
        string full_path = paths[i];
        if (full_path.back() != '/') { // for incomplete paths
            full_path += '/';
        }
        full_path += command;
        if (access(full_path.c_str(), X_OK) == 0) {
            return full_path;
        }
    }
    return "";  // command wasn't found
}

// helper function to tokenize command
bool parse_command(string& command, vector<vector<string>>& token_matrix, 
                   vector<bool>& file_red_vec, vector<string>& out_file_vec) {
    vector<string> tokens; // for storing the current line
    stringstream ss(command); // for parsing the current line
    string word; // keep track of the current word

    // for redirects (if applicable)
    bool file_redirect = false;
    string output_file = "";
    bool redirect_file_set = false;

    // read individual words from the string stream
    while (ss >> word) {
        // redirection handler
        size_t pos = word.find('>');
        if (pos != string::npos) { // redirection operator in current token
            if (file_redirect) { // error if already accounted for redirection
                print_error();
                return false;
            }

            file_redirect = true;

            // split token into left part and right part of '>'
            string left = word.substr(0, pos);
            string right = word.substr(pos + 1);

            // if there's a left part, add it as a token
            if (!left.empty()) {
                tokens.push_back(left);
            } 

            // push back redirect token
            tokens.push_back(">");

            // cannot start with redirection operator
            if (tokens[0] == ">") { 
                print_error();
                return false;
            }

            // if there's something after '>', treat it as the output file
            if (!right.empty()) {
                output_file = right;
                redirect_file_set = true;
            } else {
                // check next word for output file 
                if (!(ss >> word)) { // error: no file
                    print_error();
                    return false;
                } else {
                    // set the output file
                    if (word == ">") { // multiple redirects
                        print_error();
                        return false;
                    }
                    output_file = word;
                    redirect_file_set = true;
                    if (ss >> word) { // error: multiple file paths
                        print_error();
                        return false;
                    }
                }
            }
        } else {
            // account for multiple output files
            if (redirect_file_set) { 
                print_error();
                return false;
            }
            tokens.push_back(word);
        }
    }

    // check if output file has an > attached to it (edge case)
    size_t pos = output_file.find('>');
    if (pos != string::npos) {
        print_error();
        return false;
    }

    // everything went correct with the tokenization
    token_matrix.push_back(tokens);
    file_red_vec.push_back(file_redirect);
    out_file_vec.push_back(output_file);
    
    return true;
}

// helper function to execute correct command with given data
void execute_instruction(vector<string>& tokens, const string& output_file, 
                         const bool& file_redirect, vector<string>& paths) {

    // prepare the args array for execv
    int args_length = tokens.size() + 1;
    if (file_redirect) { // must ignore redirect in tokens
        args_length = tokens.size() - 1;
    }

    // fill out args array
    char *args[args_length];
    for (unsigned int i = 0; i < tokens.size(); i++) {
        args[i] = strdup(tokens[i].c_str());
    }

    // finish arg array with nullptr
    if (file_redirect) {
        args[tokens.size() - 1] = nullptr;
    } else {
        args[tokens.size()] = nullptr;  
    }
    
    // search for the executable in the given paths
    string exec_path = search_executable_path(args[0], paths);
    if (exec_path.empty()) {
        print_error();
        exit(1);
    }

    // handle file output redirection
    if (file_redirect) {
        int fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            print_error();
            exit(1);
        }
        dup2(fd, STDOUT_FILENO); // redirect stdout to file
        close(fd);
    }

    // execute the command
    int exec_ret = execv(exec_path.c_str(), args);
    if (exec_ret == -1) {
        print_error();
        exit(1);
    }
}

// helper function to handle single commands and parallel commands
void parse_line(string& curr_line, vector<string>& paths) {
    if (curr_line.size() == 0) { // skip empty lines
        return;
    }

    vector<pid_t> children; // vec to store current child processes
    vector<vector<string>> tokens_matrix; // vector of vector of tokens
    vector<bool> file_redir_vec; // vector for file redirect presence (if necessary)
    vector<string> output_file_vec; // vector for output files (if necessary)

    size_t pos = curr_line.find('&'); // check for parallel commands in our line
    if (pos != string::npos) { // at least 1 &
        while (pos != string::npos) { // loop until no more commands to parse
        
            // extract left command and tokenize the data
            string left_command = curr_line.substr(0, pos);
            if(!parse_command(left_command, tokens_matrix, file_redir_vec, output_file_vec)) {
                // there was an error with this command
                return;
            }

            // remove left command and continue
            curr_line = curr_line.substr(pos + 1);
            pos = curr_line.find('&');
        }

        // tokenize the final part of the current line
        if(!parse_command(curr_line, tokens_matrix, file_redir_vec, output_file_vec)) {
                // there was an error with this command
                return;
        }
        
        // iterate through all the commands and run operations in parallel
        for (unsigned int i = 0; i < tokens_matrix.size(); i++) {
            if (tokens_matrix[i].size() == 0) { // skip empty lines
                continue;
            }
            // split into parent/child process for parallel computation
            pid_t pid = fork();
            if (pid != 0) { // parent process
                // store each children's id to wait for parallel execution to complete
                children.push_back(pid);
            } else if (pid == 0) { // child process
                // pass command into execution function to be executed, upon completion exit process
                execute_instruction(tokens_matrix[i], output_file_vec[i], file_redir_vec[i], paths);
                exit(0);
            } else { // error on fork
                print_error();
                exit(1);
            }
        }

        // in the parent process, wait for all the child processes to finish
        for (unsigned int i = 0; i < children.size(); i++) {
            waitpid(children[i], NULL, 0);
        }   
    } else { // single command in line
        // tokenize command
        if(!parse_command(curr_line, tokens_matrix, file_redir_vec, output_file_vec)) {
                // there was an error with this command
                return;
        }

        if (tokens_matrix[0].size() == 0) {
            return; // skip empty lines
        }
        
        // check if the command is a built in command
        if (handle_builtin_commands(tokens_matrix[0], paths)) {
           return; 
        }

        // split into parent/child process
        pid_t pid = fork();
        if (pid != 0) { // parent process
            // store children's id to wait for parallel execution to complete
            children.push_back(pid);
            // while in the parent process, wait for all the child processes to finish
            waitpid(children[0], NULL, 0);
        } else if (pid == 0) { // child process
            // pass command into execution function to be executed, upon completion exit process
            execute_instruction(tokens_matrix[0], output_file_vec[0], file_redir_vec[0], paths);
            exit(0);
        } else { // error on fork
            print_error();
            exit(1);
        }
    }
}


int main(int argc, char* argv[]) {
    // configure shell mode
    int mode = find_mode(argc);

    // set up our paths
    vector<string> paths;
    paths.push_back("/bin/");

    if (mode == 0) {
        // interactive mode, read commands from stdin
        while (true) {
            // print prompt
            cout << "wish> ";

            string curr_line;
            // get a line from standard input
            if (!getline(cin, curr_line)) {
                break; // if we've reached eof, break
            }
            // parse the line using helper functions
            parse_line(curr_line, paths);
        }
    } else if (mode == 1) {
        // batch mode, read commands from input file
        ifstream batch_file(argv[1]);

        // error opening file
        if (!batch_file.is_open()) {
            print_error();
            exit(1);
        }

        string curr_line;
        // get a line from the batch file
        while (getline(batch_file, curr_line)) {
            parse_line(curr_line, paths);
        }
    }

    return 0;
}