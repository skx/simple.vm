//
// This is a simple port of the virtual machine to golang
//
// It is not complete, because it only operates upon some of the examples.
//
// For example the loop script:
//
//     ./compiler examples/loop.in
//     go run main.go examples/loop.raw
//
// And the jumping script:
//
//     ./compiler examples/jump.in
//     go run main.go examples/jump.raw
//
// Still it shows how you could "port" this software, if you had a pressing
// need for a Golang based VM.
//
// Steve
// --
//

package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"runtime"
)

// Flags holds the CPU flags.
type Flags struct {
	// Zero-flag
	z bool
}

// Register holds the contents of a single register.
//
// This is horrid because we don't use an enum for the type.
type Register struct {
	// Integer contents of register if t == "int"
	i int

	// String contents of register if t == "string"
	s string

	// Register type: "int" vs. "string"
	t string
}

// CPU is our virtual machine state.
type CPU struct {
	// Registers
	regs [16]Register

	// Flags
	flags Flags

	// Our RAM - where the program is loaded
	mem [0xFFFF]byte

	// Instruction-pointer
	ip int
}

func debugPrintf(fmt_ string, args ...interface{}) {
	if os.Getenv("DEBUG") == "" {
		return
	}
	programCounter, file, line, _ := runtime.Caller(1)
	fn := runtime.FuncForPC(programCounter)
	prefix := fmt.Sprintf("[%s:%s %d] %s", file, fn.Name(), line, fmt_)
	fmt.Printf(prefix, args...)
	fmt.Println()
}

// NewCPU returns a new CPU object
func NewCPU() *CPU {
	x := &CPU{}
	for i := 0; i < 16; i++ {
		x.regs[i].t = "int"
	}
	x.ip = 0

	return x
}

// Load opens the named program and loads it
func (c *CPU) Load(path string) {

	// Ensure we reset our state.
	c.ip = 0

	// Load the file
	b, err := ioutil.ReadFile(path)
	if err != nil {
		fmt.Printf("Failed to read file: %s - %s\n", path, err.Error())
		os.Exit(1)
	}

	if len(b) >= 0xFFFF {
		fmt.Printf("Program too large for RAM!\n")
		os.Exit(1)
	}

	// Copy contents of file to our memory region
	for i := 0; i < len(b); i++ {
		c.mem[i] = b[i]
	}
}

// Read a string from the IP position
// Strings are prefixed by their lengths (two-bytes).
func (c *CPU) readString() string {
	// Read the length of the string
	len := c.read2Val()

	debugPrintf("string length(%04X);\n", len)

	// Now build up the body of the string
	s := ""
	for i := 0; i < len; i++ {
		s += string(c.mem[c.ip+i])
	}

	c.ip += (len)
	return s
}

// Read a two-byte number from the current IP.
func (c *CPU) read2Val() int {
	l := int(c.mem[c.ip])
	c.ip += 1
	h := int(c.mem[c.ip])
	c.ip += 1

	val := l + h*256
	return (val)
}

// Run launches our intepreter.
func (c *CPU) Run() {

	c.ip = 0
	run := true
	for run {

		instruction := c.mem[c.ip]

		debugPrintf("About to execute instruction %02X\n", instruction)

		switch instruction {
		case 0x00:
			debugPrintf("EXIT\n")
			run = false
		case 0x01:
			debugPrintf("INT_STORE\n")

			// register
			c.ip += 1
			reg := int(c.mem[c.ip])
			c.ip += 1
			val := c.read2Val()

			debugPrintf("\tSet register %02X to %04X\n", reg, val)

			// store the int
			c.regs[reg].i = val
			// set the type
			c.regs[reg].t = "int"
		case 0x02:
			debugPrintf("INT_PRINT\n")
			// register
			c.ip += 1
			reg := c.mem[c.ip]

			fmt.Printf("%d", c.regs[reg].i)
			c.ip += 1
		case 0x10:
			debugPrintf("JUMP\n")
			c.ip += 1
			addr := c.read2Val()
			c.ip = addr
		case 0x11:
			debugPrintf("JUMP_Z\n")
			c.ip += 1
			addr := c.read2Val()
			if c.flags.z {
				c.ip = addr
			}
		case 0x12:
			debugPrintf("JUMP_NZ\n")
			c.ip += 1
			addr := c.read2Val()
			if !c.flags.z {
				c.ip = addr
			}

		case 0x21:
			debugPrintf("ADD\n")
			c.ip += 1
			res := c.mem[c.ip]
			c.ip += 1
			a := c.mem[c.ip]
			c.ip += 1
			b := c.mem[c.ip]
			c.ip += 1

			// store result
			c.regs[res].i = (c.regs[a].i + c.regs[b].i)

		case 0x22:
			debugPrintf("SUB\n")
			c.ip += 1
			res := c.mem[c.ip]
			c.ip += 1
			a := c.mem[c.ip]
			c.ip += 1
			b := c.mem[c.ip]
			c.ip += 1

			// store result
			c.regs[res].i = (c.regs[a].i - c.regs[b].i)

			// set the zero-flag if the result was zero or less
			if c.regs[res].i <= 0 {
				c.flags.z = true
			}

		case 0x30:
			debugPrintf("STORE_STRING\n")

			// register
			c.ip += 1
			reg := c.mem[c.ip]

			// bump past that to the length
			c.ip += 1
			debugPrintf("\tStoring to register %02X\n", reg)

			// length
			str := c.readString()
			debugPrintf("\tRead String: '%s'\n", str)

			// store the string
			c.regs[reg].s = str
			// set the type
			c.regs[reg].t = "string"

		case 0x31:
			debugPrintf("PRINT_STRING\n")

			// register
			c.ip += 1
			reg := c.mem[c.ip]

			fmt.Printf("%s", c.regs[reg].s)
			c.ip += 1
		case 0x50:
			debugPrintf("NOP\n")
			c.ip += 1
		default:
			debugPrintf("%04X - Instruction %02X\n", c.ip, instruction)
			os.Exit(1)
		}

		// Ensure our instruction-pointer wraps around.
		if c.ip >= 0xFFFF {
			c.ip = 0
		}
	}
}

// Main driver
func main() {

	for _, ent := range os.Args[1:] {

		fmt.Printf("Loading file %s\n", ent)
		c := NewCPU()
		c.Load(ent)

		c.Run()
	}

}
