add_subdirectory(ir)
add_subdirectory(led)
# No DCF77 driver: the on-board DCF77 receiver (LNA sheet) does not work, see the note in app/src/Main.cpp.
add_subdirectory(power)
add_subdirectory(touch)
