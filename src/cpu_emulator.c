#include <stdio.h>
#include "cpu.h"
#include "bus.h"

cpu_s *get_cpu_instance(void);
cpu_s *get_cpu_instance(void)
{
    static cpu_s cpu;
    return &cpu;
}

bus_s *get_bus_instance(void);
bus_s *get_bus_instance(void)
{
    static bus_s bus;
    return &bus;
}

int main(void)
{
    cpu_s *cpu = get_cpu_instance();
    bus_s *bus = get_bus_instance();
    bus_init(bus);
    cpu_init(cpu, bus);
    return 0;
}
