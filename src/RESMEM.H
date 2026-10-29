/* DOS allocation transaction shared with fault-injection host tests.
 * The caller supplies dos_get_strategy/set_strategy/allocate/free_block
 * and dos_valid_video_block. No MCB modifications are made here. */
static unsigned reservation, saved_strategy, strategy_pending;
static unsigned cleanup_error, last_dos_error;
static unsigned last_free_result;
static int restore_strategy(void)
{
    unsigned error;
    if (!strategy_pending) return 1;
    error = dos_set_strategy(saved_strategy);
    if (error) {
        last_dos_error = error; cleanup_error = 1; return 0;
    }
    strategy_pending = 0; return 1;
}
static int release_reserved(void)
{
    unsigned error;
    if (!reservation) return 1;
    error = dos_free_block(reservation);
    last_free_result = error;
    if (error) {
        last_dos_error = error; cleanup_error = 1; return 0;
    }
    reservation = 0; return 1;
}
static int reserve_block(void)
{
    unsigned error; int restored;
    error = dos_get_strategy(&saved_strategy);
    if (error) { last_dos_error = error; return 0; }
    error = dos_set_strategy(2);
    if (error) { last_dos_error = error; return 0; }
    strategy_pending = 1;
    error = dos_allocate(0x400, &reservation);
    if (error) { reservation = 0; last_dos_error = error; }
    restored = restore_strategy();
    if (!restored || error || reservation != 0x9800 ||
        !dos_valid_video_block(reservation)) {
        release_reserved(); return 0;
    }
    return 1;
}
