#include "WiiIR/IR.hpp"
#ifdef NINTENDOWII
#include <iostream>
#include <streambuf>
#include <string>
#include <ogc/system.h>

// Custom streambuf that writes to SYS_Report
class OSReportStreambuf : public std::streambuf {
protected:
    virtual int_type overflow(int_type c) {
        if (c != EOF) {
            // Append the character to a temporary buffer
            buffer_ += static_cast<char>(c);
            // If it's a newline, flush the buffer to OSReport
            if (c == '\\n') {
                sync();
            }
        }
        return c;
    }

    virtual int sync() {
        if (!buffer_.empty()) {
            // Use SYS_Report to write the accumulated string
            SYS_Report(buffer_.c_str());
            buffer_.clear();
        }
        return 0;
    }

private:
    std::string buffer_;
};

// Global pointers to save the original buffers
static std::streambuf *original_cerr_buffer = nullptr;
static std::streambuf *original_cout_buffer = nullptr;
OSReportStreambuf osreport_streambuf;

void setup_osreport_redirection() {
    // Save the original buffer so we can restore it later if needed
    original_cout_buffer = std::cout.rdbuf();
    original_cerr_buffer = std::cerr.rdbuf();

    // Set the new buffer for std::cerr
    std::cout.rdbuf(&osreport_streambuf);
    std::cerr.rdbuf(&osreport_streambuf);

    // Print notification
    std::cout << __FUNCTION__ << "() [INFO]: Using SYS_Report() for std::cerr and std::cout" << std::endl;
}

void restore_original_cerr() {
    // Print notification
    std::cerr << __FUNCTION__ << "() [INFO]: No longer directing std::cerr to SYS_Report()" << std::endl;

    // Restore the original buffer
    if (original_cerr_buffer) {
        std::cerr.rdbuf(original_cerr_buffer);

        #ifdef DEBUG
            std::cerr << __FUNCTION__ << "(): Success" << std::endl;
        #endif
    } else {
        SYS_Report("Error: original_cerr_buffer was NULL, cannot restore.\n");
    }
}

void restore_original_cout() {
    // Print notification
    std::cout << __FUNCTION__ << "() [INFO]: No longer directing std::cout to SYS_Report()" << std::endl;

    // Restore the original buffer
    if (original_cout_buffer) {
        std::cout.rdbuf(original_cout_buffer);
        
        // Debug Message
        #ifdef DEBUG
            std::cout << __FUNCTION__ << "(): Success" << std::endl;
        #endif
    } else {
        SYS_Report("Error: original_cout_buffer was NULL, cannot restore.\n");
    }
}
#else
#include <stdio.h>
void setup_osreport_redirection() {
    printf("%s() [INFO]: Not needed on this platform.\n", __FUNCTION__);
} void restore_original_cerr() {
    printf("%s() [INFO]: Not needed on this platform.\n", __FUNCTION__);
} void restore_original_cout() {
    printf("%s() [INFO]: Not needed on this platform.\n", __FUNCTION__);
}
#endif