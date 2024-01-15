#include <iostream>
#include <fstream>

class Timestamps {
public:
        uint64_t creation;
        uint64_t xfer_start;
        uint64_t xfer_prp;
        uint64_t xfer_end;
        uint64_t backend_start;
        uint64_t backend_end;
        uint64_t completion_start;
        //uint64_t irq_start;
        uint64_t completion;
        uint64_t put_in_user_queue;
        uint64_t user_space_start;
        uint64_t user_space_end;

        static void print_csv_header() {
                std::cout << "creation, xfer_start, xfer_prp, xfer_end, backend_start, "
                             "backend_end, completion_start, completion, put_in_user_queue, user_space_start, user_space_end"
                          << std::endl;
        }

        void print_as_csv_line() const {
                std::cout         << creation
                          << ", " << xfer_start
                          << ", " << xfer_prp
                          << ", " << xfer_end
                          << ", " << backend_start
                          << ", " << backend_end
                          << ", " << completion_start /* << ", " << irq_start */
                          << ", " << completion
                          << ", " << put_in_user_queue
                          << ", " << user_space_start
                          << ", " << user_space_end
                          << std::endl;
        }
};

int main(int argc, const char *argv[]) {

        Timestamps ts[1000];

        std::ifstream in(argv[1], std::ios::binary);

        Timestamps::print_csv_header();

        for (int i = 0; i < 1000; ++i) {
                in.read((char *)&ts[i], sizeof(ts[i]));
                ts[i].print_as_csv_line();
        }

        return 0;
}