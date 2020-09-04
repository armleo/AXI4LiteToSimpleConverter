#include <verilated.h>
#include <verilated_vcd_c.h>
#include <VAXI4LiteConverter.h>
#include <iostream>

vluint64_t simulation_time = 0;
VerilatedVcdC	*m_trace;
bool trace = 1;
VAXI4LiteConverter* converter;

using namespace std;

double sc_time_stamp() {
    return simulation_time;  // Note does conversion to real, to match SystemC
}
void dump_step() {
    simulation_time++;
    if(trace) {
        m_trace->dump(simulation_time);
        //m_trace->flush();
        //cout << "Dump step" << endl << flush;
    }
}
void update() {
    converter->eval();
    dump_step();
}

void posedge() {
    converter->clk = 1;
    update();
    update();
}

void till_user_update() {
    converter->clk = 0;
    update();
}
void after_user_update() {
    update();
}


void next_cycle() {
    after_user_update();

    posedge();
    till_user_update();
    converter->eval();
}


string testname;
int testnum;

void test_begin(int num, string tn) {
    testname = tn;
    testnum = num;
    cout << testnum << " - " << testname << endl;
}

void test_end() {
    next_cycle();
    cout << testnum << " - " << testname << " DONE" << endl;
}

#define check(expr) {if(!(expr)) {cout << "Testnum: " << testnum << " Check failed: " #expr "" << endl; throw runtime_error("Failed: " #expr "");}}


int main(int argc, char** argv, char** env) {
    cout << "Test started" << endl;
    // This is a more complicated example, please also see the simpler examples/make_hello_c.

    // Prevent unused variable warnings
    if (0 && argc && argv && env) {}

    // Set debug level, 0 is off, 9 is highest presently used
    // May be overridden by commandArgs
    Verilated::debug(0);

    // Randomization reset policy
    // May be overridden by commandArgs
    Verilated::randReset(2);

    // Verilator must compute traced signals
    Verilated::traceEverOn(true);

    // Pass arguments so Verilated code can see them, e.g. $value$plusargs
    // This needs to be called before you create any model
    Verilated::commandArgs(argc, argv);

    // Create logs/ directory in case we have traces to put under it
    Verilated::mkdir("logs");

    // Construct the Verilated model, from Vaxi4liteconverter.h generated from Verilating "converter.v"
    converter = new VAXI4LiteConverter;  // Or use a const unique_ptr, or the VL_UNIQUE_PTR wrapper
    m_trace = new VerilatedVcdC;
    converter->trace(m_trace, 99);
    m_trace->open("vcd_dump.vcd");


    try {
        cout << "Starting tests" << endl;
        converter->rst_n = 0;
        converter->AXI_ARVALID = 0;
        converter->AXI_AWVALID = 0;
        converter->AXI_WVALID = 0;
        converter->AXI_BREADY = 0;
        converter->AXI_RREADY = 0;

        converter->address_error = 0;
        converter->write_error = 0;
        converter->read_data = 0;

        till_user_update();
        cout << "First eval successful" << endl;
        converter->rst_n = 0;
        next_cycle();


        test_begin(0, "NOOP Check with AWVALID = 0; WVALID = 0");
        converter->rst_n = 1;
        converter->AXI_AWVALID = 0;
        converter->AXI_WVALID = 0;
        converter->AXI_AWADDR = 4;
        converter->AXI_ARADDR = 5;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_ARREADY == 0);
        check(converter->AXI_BVALID == 0);
        check(converter->AXI_RVALID == 0);

        check(converter->write == 0);
        check(converter->read == 0);

        test_begin(1, "NOOP Check with AWVALID = 1; WVALID = 0");
        converter->rst_n = 1;
        converter->AXI_AWVALID = 1;
        converter->AXI_WVALID = 0;
        converter->AXI_AWADDR = 4;
        converter->AXI_ARADDR = 5;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_ARREADY == 0);
        check(converter->AXI_BVALID == 0);
        check(converter->AXI_RVALID == 0);

        check(converter->write == 0);
        check(converter->read == 0);

        test_begin(2, "NOOP Check with AWVALID = 0; WVALID = 1");
        converter->rst_n = 1;
        converter->AXI_AWVALID = 0;
        converter->AXI_WVALID = 1;
        converter->AXI_AWADDR = 4;
        converter->AXI_ARADDR = 5;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_ARREADY == 0);
        check(converter->AXI_BVALID == 0);
        check(converter->AXI_RVALID == 0);

        check(converter->write == 0);
        check(converter->read == 0);


        
        next_cycle();
        test_begin(101, "Write request");
        converter->AXI_WVALID = 1;
        converter->AXI_AWVALID = 1;
        converter->AXI_WDATA = 0xFF00FF00;
        converter->AXI_WSTRB = 0xF;
        converter->eval();
        check(converter->AXI_AWREADY == 1);
        check(converter->AXI_WREADY == 1);
        check(converter->AXI_BVALID == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 0);

        check(converter->write == 1);
        check(converter->write_data == 0xFF00FF00);
        check(converter->address == converter->AXI_AWADDR);
        check(converter->write_byteenable == converter->AXI_WSTRB);
        check(converter->read == 0);
        
        test_begin(102, "Write request done");
        next_cycle();
        converter->AXI_WVALID = 0;
        converter->AXI_BREADY = 1;
        converter->AXI_AWVALID = 0;

        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 0);
        check(converter->AXI_BVALID == 1);
        check(converter->AXI_BRESP == 0b00);
        check(converter->write == 0);
        check(converter->read == 0);
        
        next_cycle();
        test_begin(103, "Write request, w/ Address error");
        converter->AXI_WVALID = 1;
        converter->AXI_WDATA = 0xFF00FF00;
        converter->AXI_WSTRB = 0xF;
        converter->address_error = 1;
        converter->AXI_AWVALID = 1;
        converter->AXI_AWADDR = 100;
        converter->eval();
        check(converter->AXI_AWREADY == 1);
        check(converter->AXI_WREADY == 1);
        check(converter->AXI_BVALID == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 0);

        check(converter->write == 1);
        check(converter->write_data == 0xFF00FF00);
        check(converter->address == converter->AXI_AWADDR);
        check(converter->write_byteenable == converter->AXI_WSTRB);
        check(converter->read == 0);
        
        test_begin(104, "Write request done, w/ Address error");
        next_cycle();
        converter->AXI_WVALID = 0;
        converter->AXI_AWVALID = 0;
        converter->address_error = 0;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 0);
        check(converter->AXI_BVALID == 1);
        check(converter->AXI_BRESP == 0b10);
        check(converter->write == 0);
        check(converter->read == 0);


        next_cycle();
        test_begin(105, "Write request, w/ missalligned Address error");
        converter->AXI_WVALID = 1;
        converter->AXI_WDATA = 0xFF00FF00;
        converter->AXI_WSTRB = 0xF;
        converter->address_error = 0;
        converter->AXI_AWVALID = 1;
        converter->AXI_AWADDR = 101;
        converter->eval();
        check(converter->AXI_AWREADY == 1);
        check(converter->AXI_WREADY == 1);
        check(converter->AXI_BVALID == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 0);

        check(converter->write == 1);
        check(converter->write_data == 0xFF00FF00);
        check(converter->address == converter->AXI_AWADDR);
        check(converter->write_byteenable == converter->AXI_WSTRB);
        check(converter->read == 0);
        
        test_begin(106, "Write request done, w/ Address error");
        next_cycle();
        converter->AXI_WVALID = 0;
        converter->AXI_AWVALID = 0;
        converter->address_error = 0;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 0);
        check(converter->AXI_BVALID == 1);
        check(converter->AXI_BRESP == 0b10);
        check(converter->write == 0);
        check(converter->read == 0);

        next_cycle();
        test_begin(107, "Write request, w/ error");
        converter->AXI_WVALID = 1;
        converter->AXI_WDATA = 0xFF00FF00;
        converter->AXI_WSTRB = 0xF;
        converter->write_error = 1;
        converter->AXI_AWVALID = 1;
        converter->AXI_AWADDR = 100;
        converter->eval();
        check(converter->AXI_AWREADY == 1);
        check(converter->AXI_WREADY == 1);
        check(converter->AXI_BVALID == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 0);

        check(converter->write == 1);
        check(converter->write_data == 0xFF00FF00);
        check(converter->address == converter->AXI_AWADDR);
        check(converter->write_byteenable == converter->AXI_WSTRB);
        check(converter->read == 0);
        
        test_begin(108, "Write request done, w/ error");
        next_cycle();
        converter->AXI_WVALID = 0;
        converter->AXI_AWVALID = 0;
        converter->write_error = 0;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 0);
        check(converter->AXI_BVALID == 1);
        check(converter->AXI_BRESP == 0b11);
        check(converter->write == 0);
        check(converter->read == 0);

        



        next_cycle();
        test_begin(201, "Read request");
        converter->AXI_ARVALID = 1;
        converter->AXI_ARADDR = 104; // 4 byte aligned
        converter->read_data = 0x00FF00FF;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_BVALID == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 1);

        check(converter->write == 0);
        check(converter->read == 1);
        check(converter->address == converter->AXI_ARADDR);

        
        test_begin(202, "Read request done");
        next_cycle();
        converter->AXI_ARVALID = 0;
        converter->read_data = 0x00FF0000;
        converter->address_error = 0;
        converter->AXI_RREADY = 1;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_RVALID == 1);
        check(converter->AXI_RRESP == 0b00);
        check(converter->AXI_RDATA == 0x00FF00FF);
        check(converter->AXI_ARREADY == 0);
        check(converter->AXI_BVALID == 0);
        check(converter->write == 0);
        check(converter->read == 0);





        next_cycle();
        test_begin(203, "Read request w/ address error");
        converter->AXI_ARVALID = 1;
        converter->AXI_ARADDR = 104; // 4 byte aligned
        converter->read_data = 0x00FF00FF;
        converter->address_error = 1;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_BVALID == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 1);

        check(converter->write == 0);
        check(converter->read == 1);
        check(converter->address == converter->AXI_ARADDR);

        
        test_begin(204, "Read request done w/ address error");
        next_cycle();
        converter->AXI_ARVALID = 0;
        converter->read_data = 0x00FF0000;
        converter->address_error = 0;
        converter->AXI_RREADY = 1;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_RVALID == 1);
        check(converter->AXI_RRESP == 0b10);
        check(converter->AXI_RDATA == 0x00FF00FF);
        check(converter->AXI_ARREADY == 0);
        check(converter->AXI_BVALID == 0);
        check(converter->write == 0);
        check(converter->read == 0);



        next_cycle();
        test_begin(205, "Read request w/ missaligned address error");
        converter->AXI_ARVALID = 1;
        converter->AXI_ARADDR = 101; // 4 byte aligned
        converter->read_data = 0x00FF00FF;
        converter->address_error = 1;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_BVALID == 0);
        check(converter->AXI_RVALID == 0);
        check(converter->AXI_ARREADY == 1);

        check(converter->write == 0);
        check(converter->read == 1);
        check(converter->address == converter->AXI_ARADDR);

        
        test_begin(206, "Read request done w/ missaligned address error");
        next_cycle();
        converter->AXI_ARVALID = 0;
        converter->read_data = 0x00FF0000;
        converter->address_error = 0;
        converter->AXI_RREADY = 1;
        converter->eval();
        check(converter->AXI_AWREADY == 0);
        check(converter->AXI_WREADY == 0);
        check(converter->AXI_RVALID == 1);
        check(converter->AXI_RRESP == 0b10);
        check(converter->AXI_RDATA == 0x00FF00FF);
        check(converter->AXI_ARREADY == 0);
        check(converter->AXI_BVALID == 0);
        check(converter->write == 0);
        check(converter->read == 0);

        
    } catch(runtime_error e) {
        cout << e.what();
        cout << endl << "Error intercepted" << endl << flush;
        
    }
    next_cycle();
    next_cycle();
    converter->final();
    if (m_trace) {
        m_trace->flush();
        m_trace->close();
        delete m_trace;
        m_trace = NULL;
    }
#if VM_COVERAGE
    Verilated::mkdir("logs");
    VerilatedCov::write("logs/coverage.dat");
#endif

    // Destroy model
    delete converter; converter = NULL;

    // Fin
    exit(0);
}