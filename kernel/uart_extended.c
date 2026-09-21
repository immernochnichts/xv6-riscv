//
// low-level driver for 16550a UART.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "defs.h"

// the UART control registers are memory-mapped
// at address UART0. this macro returns the
// address of one of the registers.
#define UartId(id) ( (id) == 0 ? (UART0) : (UART1) )
#define Reg(reg, uart_id) ((volatile unsigned char *)(UartId(uart_id) + (reg)))

#define ReadReg(reg, uart_id)     (*(Reg(reg, uart_id)))
#define WriteReg(reg, v, uart_id) (*(Reg(reg, uart_id)) = (v))

// the UART control registers.
// some have different meanings for read vs write.
// see http://byterunner.com/16550.html
#define RHR             0        // receive holding register (for input bytes)
#define THR             0        // transmit holding register (for output bytes)
#define IER             1        // interrupt enable register
#define IER_RX_ENABLE   (1 << 0) // receiver interrupts
#define IER_TX_ENABLE   (1 << 1) // transmit interrupts
#define FCR             2        // FIFO control register
#define FCR_FIFO_ENABLE (1 << 0)
#define FCR_FIFO_CLEAR  (3 << 1) // clear the content of the two FIFOs
#define ISR             2        // interrupt status register
#define LCR             3        // line control register
#define LCR_EIGHT_BITS  (3 << 0)
#define LCR_BAUD_LATCH  (1 << 7) // special mode to set baud rate
#define LSR             5        // line status register
#define LSR_RX_READY    (1 << 0) // input is waiting to be read from RHR
#define LSR_TX_IDLE     (1 << 5) // THR can accept another character to send

// for sending threads to serialize their writes
static struct sleeplock tx_lock[2];
static int tx_chan[2]; // &tx_chan is the "wait channel"

extern volatile int panicking; // from printk.c
extern volatile int panicked;  // from printk.c

void
uartinit_x(uint8 uart_id)
{
  if (uart_id > 1)
    panic("bad uart id");

  // disable interrupts.
  WriteReg(IER, 0x00, uart_id);

  // special mode to set baud rate.
  WriteReg(LCR, LCR_BAUD_LATCH, uart_id);

  // LSB for baud rate of 38.4K.
  WriteReg(0, 0x03, uart_id);

  // MSB for baud rate of 38.4K.
  WriteReg(1, 0x00, uart_id);

  // leave set-baud mode,
  // and set word length to 8 bits, no parity.
  WriteReg(LCR, LCR_EIGHT_BITS, uart_id);

  // reset and enable FIFOs.
  WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR, uart_id);

  // enable transmit and receive interrupts.
  WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE, uart_id);

  initsleeplock(&tx_lock[uart_id], "uart");
}

// transmit buf[] to the uart. it blocks if the
// uart is busy, so it cannot be called from
// interrupts, only from write() system calls.
void
uartwrite_x(char buf[], int n, uint8 uart_id)
{
  if (uart_id > 1)
    panic("bad uart id");

  acquiresleep(&tx_lock[uart_id]);

  int i = 0;
  while (i < n) {
    sleep_prepare(&tx_chan[uart_id]);
    if (ReadReg(LSR, uart_id) & LSR_TX_IDLE) {
      WriteReg(THR, buf[i], uart_id);
      i += 1;
    } else {
      sleep();
    }
  }

  releasesleep(&tx_lock[uart_id]);
}

// write a byte to the uart without using
// interrupts, for use by kernel printk() and
// to echo characters. it spins waiting for the uart's
// output register to be empty.
void
uartputc_sync_x(int c, uint8 uart_id)
{
  if (uart_id > 1)
    panic("bad uart id");

  if (panicking == 0)
    push_off();

  if (panicked) {
    for (;;)
      ;
  }

  // wait for UART to set Transmit Holding Empty in LSR.
  while ((ReadReg(LSR, uart_id) & LSR_TX_IDLE) == 0)
    ;
  WriteReg(THR, c, uart_id);

  if (panicking == 0)
    pop_off();
}

// try to read one input character from the UART.
// return -1 if none is waiting.
static int
uartgetc_x(uint8 uart_id)
{
  if (uart_id > 1)
    panic("bad uart id");

  // is input ready?
  if (ReadReg(LSR, uart_id) & LSR_RX_READY) {
    return ReadReg(RHR, uart_id);
  } else {
    return -1;
  }
}

// handle a uart interrupt, raised because input has
// arrived, or the uart is ready for more output, or
// both. called from devintr().
void
uartintr_x(uint8 uart_id)
{
  if (uart_id > 1)
    panic("bad uart id");

  ReadReg(ISR, uart_id); // acknowledge the interrupt

  if (ReadReg(LSR, uart_id) & LSR_TX_IDLE) {
    // UART finished transmitting; wake up sending thread.
    wakeup(&tx_chan[uart_id]);
  }

  // read and process incoming characters, if any.
  while (1) {
    int c = uartgetc_x(uart_id);
    if (c == -1)
      break;
    if (uart_id == 0) // redirect incoming characters to console via uart0
      consoleintr(c);
  }
}

// wrappers with old signatures to keep old api working
void
uartinit()
{
  uartinit_x(0);
  uartinit_x(1);
}

void
uartwrite(char buf[], int n)
{
  uartwrite_x(buf, n, 0);
}

void
uartputc_sync(int c)
{
  uartputc_sync_x(c, 0);
}

void
uartintr(void)
{
  uartintr_x(0);
}