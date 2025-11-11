#include <iostream>

int main(const int argc, char* argv[] ) {
    //Must be only one argument provided
    if (argc != 2) {
        std::cerr << "Usage: <program> <hostname>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string_view hostname = argv[1];

    //Reject blank argument
    if (hostname.empty()) return EXIT_FAILURE;
    //Reject too large argument
    if (hostname.size() > 253) return EXIT_FAILURE;
    //Reject special hostname symbol
    if (hostname.find("://") != std::string_view::npos) {
        std::cerr << "Invalid input: looks like a URL (contains \"://\"). Please pass a hostname like \"example.com\"." << std::endl;
        return EXIT_FAILURE;
    }
    //Reject path or query chars
    if (
        hostname.find('/') != std::string_view::npos ||
        hostname.find('\\') != std::string_view::npos ||
        hostname.find('?') != std::string_view::npos ||
        hostname.find('&') != std::string_view::npos ||
        hostname.find('#') != std::string_view::npos
    ) {
        std::cerr << "Invalid input: contains path or query characters ('/', '?', '#')" << std::endl;
        return EXIT_FAILURE;
    }

    if (
        hostname.find(' ') < hostname.size() ||
        hostname.find('\t') < hostname.size() ||
        hostname.find('\n') < hostname.size()
    ) {
        std::cerr << "No whitespace allowed" << std::endl;
        return EXIT_FAILURE;
    }

    //Create a view which will be used to determine the correctness of the hostname
    std::string_view view = hostname;
    //In case if view ends with '.' then remove it for next checkup stages
    if (view.ends_with('.')) {
        //If the size of the view is 1, then the resulting string is empty and should be rejected
        if (view.size() == 1) {
            std::cerr << "Invalid hostname: empty label (consecutive or leading dot)." << std::endl;
            return EXIT_FAILURE;
        }
        view.remove_suffix(1);
    }

    //Initialize iterators used in splitting strings into labels
    //First label start is start of the string
    size_t labelStart = 0;
    size_t labelEnd = 0;
    do {
        //Find where the label ends (it's either next dot or end of string if no dots remain)
        labelEnd = view.find('.',labelStart);
        //Save label in separate string_view
        std::string_view labelView = view.substr(labelStart,labelEnd-labelStart);
        //Reject incorrect labels
        //Empty label
        if (labelView.empty()) {
            std::cerr << "Invalid hostname: empty label (consecutive or leading dot)." << std::endl;
            return EXIT_FAILURE;
        }
        //Label size 1-63
        if (labelView.size() > 63) {
            std::cerr << "Invalid hostname: Label length: 1–63." << std::endl;
            return EXIT_FAILURE;
        }
        //Label should not start or end with '-'
        if (labelView.ends_with('-') || labelView.starts_with('-')) {
            std::cerr << "Invalid hostname: No leading or trailing - in label." << std::endl;
            return EXIT_FAILURE;
        }

        if (!std::ranges::all_of(labelView, [](unsigned char c){return std::isalnum(c) || c == '-';})) {
            std::cerr << "Invalid hostname: Allowed chars per label: [A–Z a–z 0–9 -]." << std::endl;
            return EXIT_FAILURE;
        }

        //In case if it's the last label
        if (labelEnd == std::string_view::npos) {break;}
        //When looking for next label then it starts with the next char after the end of last one
        labelStart = labelEnd + 1;
    } while (true);

    std::cout << "Host: " << argv[1] << std::endl;
    return 0;
}