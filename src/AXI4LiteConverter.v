module AXI4LiteConverter(
    clk, rst_n,
    AXI_AWADDR, AXI_AWVALID, AXI_AWREADY,
    AXI_WDATA, AXI_WSTRB, AXI_WVALID, AXI_WREADY,
    AXI_BRESP, AXI_BVALID, AXI_BREADY,
    AXI_ARADDR, AXI_ARVALID, AXI_ARREADY,
    AXI_RDATA, AXI_RRESP, AXI_RVALID, AXI_RREADY,
    write, write_address, write_data, write_byteenable, write_address_error, write_error,
    read, read_address, read_data, read_address_error
);


    input clk;
    input rst_n;


    // address write bus
    input [31:0]                AXI_AWADDR;
    input                       AXI_AWVALID;
    output reg                  AXI_AWREADY;
    


    // Write bus
    input  [31:0]               AXI_WDATA;
    input   [3:0]               AXI_WSTRB;
    input                       AXI_WVALID;
    output reg                  AXI_WREADY;

    // Burst response bus
    output reg [1:0]            AXI_BRESP;
    reg [1:0]                   AXI_BRESP_nxt;
    output reg                  AXI_BVALID;
    input                       AXI_BREADY;


    // Address read bus
    input  [31:0]               AXI_ARADDR;
    input                       AXI_ARVALID;
    output reg                  AXI_ARREADY;

    // Read data bus
    output reg [31:0]           AXI_RDATA;
    output reg [1:0]            AXI_RRESP;
    reg [1:0]                   AXI_RRESP_nxt;
    output reg                  AXI_RVALID;
    input                       AXI_RREADY;

	
	output reg		write;
    output [31:0] 	write_address;
	output [31:0]	write_data;
    output [3:0]    write_byteenable;
    input           write_address_error;
    input           write_error;

    output reg		read; // used to retire read from register
	output [31:0]	read_address;
	input  [31:0]	read_data; // should not care about read request, always contains data accrding to read_address or address_error is asserted
    input           read_address_error; // should be valid 
	


reg [31:0] saved_readdata; // Used to ensure that data does not change
reg [31:0] saved_readdata_nxt;
reg [1:0] state;
reg [1:0] state_nxt;  // COMB

localparam STATE_ACTIVE = 2'd0,
    STATE_READ_RESPOND = 2'd1,
    STATE_WRITE_RESPOND = 2'd2;
assign write_address = AXI_AWADDR;
assign write_data = AXI_WDATA;
assign write_byteenable = AXI_WSTRB;
assign read_address = AXI_ARADDR;


always @(posedge clk) begin : main_always_ff
    if(!rst_n) begin
        AXI_RRESP <= 0;
        AXI_BRESP <= 0;
    end else begin
        state <= state_nxt;
        AXI_RRESP <= AXI_RRESP_nxt;
        AXI_BRESP <= AXI_BRESP_nxt;
        saved_readdata <= saved_readdata_nxt;
    end
end


always @* begin : main_always_comb
    AXI_AWREADY = 0;
    AXI_ARREADY = 0;
    state_nxt = state;
    saved_readdata_nxt = saved_readdata;
    AXI_WREADY = 0;
    AXI_BVALID = 0;
    AXI_BRESP_nxt = AXI_BRESP;
    AXI_RRESP_nxt = AXI_RRESP;
    AXI_RVALID = 0;
    AXI_RDATA = saved_readdata;
    write = 0;
    read = 0;
    case(state)
        STATE_ACTIVE: begin
            if(AXI_AWVALID && AXI_WVALID) begin
                AXI_AWREADY = 1; // Address write request accepted
                AXI_WREADY = 1;
                AXI_BRESP_nxt = 2'b10;
                if(write_address_error)
                    AXI_BRESP_nxt = 2'b10;
                else if(write_error)
                    AXI_BRESP_nxt = 2'b11;
                else if(AXI_AWADDR[1:0] == 2'b00) // Alligned only
                    AXI_BRESP_nxt = 2'b00;
                state_nxt = STATE_WRITE_RESPOND;
                write = 1;
            end else if(AXI_ARVALID) begin
                AXI_ARREADY = 1;
                if(AXI_ARADDR[1:0] == 2'b00 && !read_address_error)
                    AXI_RRESP_nxt = 2'b00;
                else
                    AXI_RRESP_nxt = 2'b10;
                saved_readdata_nxt = read_data;
                state_nxt = STATE_READ_RESPOND;
                read = 1;
            end
        end
        STATE_WRITE_RESPOND: begin
            AXI_BVALID = 1;
            // BRESP is already set in previous stage
            if(AXI_BREADY) begin
                state_nxt = STATE_ACTIVE;
            end
            
        end
        STATE_READ_RESPOND: begin
            AXI_RVALID = 1;
            if(AXI_RREADY) begin
                state_nxt = STATE_ACTIVE;
            end
            AXI_RDATA = saved_readdata;
        end
        default: begin
            
        end
    endcase
end

endmodule
