Extra: 

how to redirect background program output to a file
some_cmd > some_file 2>&1 &


-valgrind find memory leak
valgrind --tool=memcheck --leak-check=full ./a.out -p2005 -w200 -q200
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./appservice -p2005 -w400 -q400 -d -e

valgrind background attach
nohup bash -c "valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./tetris_services -p2005 -w400 -q400 -d -e" > /home/datafeedx11/valgrind_server.log &


-memleax find memory leak
sudo memleax -e 7 <pid>
