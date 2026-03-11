#!/bin/bash
gcc src/main.c src/login.c src/shows.c src/booking.c src/tickets.c src/waitlist.c src/bfs.c src/csv_utils.c -o theatre_app -I include
if [ $? -eq 0 ]; then
    echo "Build successful! Run ./theatre_app to start."
else
    echo "Build failed."
fi
