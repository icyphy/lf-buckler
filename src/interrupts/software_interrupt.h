#pragma once

void software_interrupt_init(void) {
    NRF_EGU1->INTENSET = 0x1;
    NVIC_EnableIRQ(SWI1_EGU1_IRQn);
}

void software_interrupt_generate(void) {
    NRF_EGU1->TASKS_TRIGGER[0] = 1;
}
