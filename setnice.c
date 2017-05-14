#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
  if(argc < 3){
    printf(2, "usage: setnice...\n");
    exit();
  }
  if(argv[2][0] == '-')
  	setnice(atoi(argv[1]), -1);
  else
  	setnice(atoi(argv[1]), atoi(argv[2]));
  exit();
}
