#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "fdy_array.h"
// #include <sys/time.h>

// fdy_arr is so far the better algorithm ## commented on 1 Oct 2016
// If sharing with multithreading, please use lock and unlock.
static FARR_MALLOC farr_malloc = malloc;
static FARR_FREE farr_free = free;
static ERR_P err_p = perror;


void farr_set_allocation_functions(FARR_MALLOC malloc_fun, FARR_FREE free_fun) {
  farr_malloc = malloc_fun;
  farr_free = free_fun;
}

void farr_set_err_functions(ERR_P err_fun) {
  err_p = err_fun;
}


fdy_arr* new_farr(size_t total_, size_t struct_size) {

  fdy_arr *farr_;

  farr_ = farr_malloc(sizeof(fdy_arr));
  if (farr_ == NULL) {
    return NULL;
  }

  farr_->_bucket_ = farr_malloc(total_ * struct_size);
  if (farr_->_bucket_ == NULL) {
    return NULL;
  }

  farr_->used_ = 0;//initiazlied as 0
  farr_->st_sz = struct_size;
  farr_->total_ = total_; //total_ count to allocated

#ifdef SYNC
  farr_->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER; //pthread_mutex_init(&(sarray->mutex), NULL);
#endif
  return farr_; //返回数组头的起始位置

}

void* farr_assign(fdy_arr *farr) {
  void * return_ = NULL;
  if (farr) {
    if (farr->used_ == farr->total_) {
      farr->total_ *= 2;

      void *newptr = farr_malloc(farr->total_ * farr->st_sz);
      memcpy(newptr, farr->_bucket_, farr->used_ * farr->st_sz);
      farr_free(farr->_bucket_);
      farr->_bucket_ = newptr; //pass back
    }

    return_ = (void *)((uintptr_t) farr->_bucket_ + farr->used_++ * farr->st_sz);
  }
  return return_;
}

/***USE AS OWN RISK WHEN MULTITHREADING, better lock***/
int farr_get(fdy_arr *farr, unsigned int index, void* return_) {
  if (farr && farr->used_ > index) {
    index--;
    memcpy(return_, (void*)((uintptr_t) farr->_bucket_ + index * farr->st_sz), farr->st_sz);
    return FARR_OK;
  }
  return FARR_NOTOK;
}

/***USE AS OWN RISK WHEN MULTITHREADING, better lock***/
int farr_remove(fdy_arr *farr, unsigned int index, void* return_) {
  if (farr && farr->used_ > index) {
    index--;
    void *pointing, *overlap;
    overlap = pointing = (void*)((uintptr_t) farr->_bucket_ + index * farr->st_sz);

    memcpy(return_, pointing, farr->st_sz);

    int afterCount = farr->total_ - index - 1;
    memmove(pointing, (void*)((uintptr_t)overlap + farr->st_sz), afterCount * farr->st_sz);
    farr->used_--;
    return 1;
  }
  return 0;
}

void free_farr(fdy_arr *farr) {
  if (farr) {
    if (farr->_bucket_) {
      farr_free(farr->_bucket_);
    }
    farr_free(farr);
  }
}

void* farr_dup_data_arr(fdy_arr *farr)  {
  void* target = farr_malloc((farr->used_ + 1) * farr->st_sz);
  memcpy(target, farr->_bucket_, farr->used_ * farr->st_sz);
  return target;
}

// void farr_select(fdy_arr *farr, int (*condition)(void* o)){
  

// }

#ifdef SYNC

void farr_lock(fdy_arr *farr) {
  pthread_mutex_lock(&(farr->mutex));
}

void farr_unlock(fdy_arr *farr) {
  pthread_mutex_unlock(&(farr->mutex));
}
#endif


// int main() {

//   // farr_set_allocation_functions(malloc, free);
//   char *strings[] = { "KimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKimKim",
//                       "ZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorroZorro",
//                       "AlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlexAlex",
//                       "CelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCelineCeline",
//                       "BillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBillBill",
//                       "ForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForestForest",
//                       "DexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexterDexter",
//                       "X PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX Pass PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX Pass PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX Pass PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX Pass PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX Pass PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX Pass PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX PassX Pass",
//                       "zurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazurianazuriana",
//                       "Abdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainalAbdul zainal",
//                       "JazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazzJazz"
//                     };

//   fdy_arr *data_arr = new_farr(300, sizeof(Node));

//   Node *node = NULL;
//   Node *emptyNode = NULL;

//   int x = 0;
//   if (node) {
//     printf("%s\n", "node is not null");
//   } else {
//     printf("%s\n", "node is null");
//   }

//   for ( x = 0; x < 11; x++ ) {
//     node = (Node*)farr_assign(data_arr);
//     if (node) {
//       printf("%s\n", "node is not null");
//       printf("node address is %d\n", node);
//       printf("%s\n", "node->data is null");
//       node->data = malloc(strlen(strings[x]) + 1);
//       if (node->data) {
//         printf("x is %d\n", x);
//         // node->data = x % 2 == 0 ? "asdasdasd" : "bbbbbb";
//         memcpy(node->data, strings[x], strlen(strings[x]));
//         // strcpy(node->data, "asdasd");
//         printf("%s\n", "node->data is not null");
//         printf("node->data is %s\n", node->data);

//       }
//     }
//   }

//   printf("total_ used_ count %d\n", data_arr->used_ );

//   Node *node_aa;
//   farr_sort_(data_arr, simple_sort);



//   // return 0;

// // // RESET:
//   node_aa = (void*)data_arr->_pool_;// + data_arr->st_sz*data_arr->count;
//   /***Alternative way***/
//   // Node *node_aa = (char *)data_arr->_pool_ + data_arr->st_sz * 1; // for index 1


//   printf("%s\n", "Starting loop data");

//   size_t count = data_arr->used_;

//   // while (count-- > 0) {
//   int i;

//   for (i = 0; i < count; i++) {
//     printf("%d\n", (void*) node_aa);
//     printf("node_aa->data is %s\n", (node_aa++)->data);
//   }

//   printf("now data arr pointing is %d\n", node_aa);

//   printf("data_arr->_pool_ is %d\n", data_arr->_pool_);

//   printf("total_ is %d\n", data_arr->total_);

//   printf("%s\n", "done");


//   farr_loop(data_arr, &print_all_v);
//   farr_loop(data_arr, &clean_all_data);


//   performance_test(data_arr);
//   // performance_casting_test(data_arr);

//   free_farr(data_arr);

//   // Node* xnode = malloc(sizeof(Node));

//   // xnode->data = "Other Value";
//   // farr_push_(data_arr, xnode);

//   // farr_free(xnode);

// // RESET:

//   return 0;
// }