// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#define NBUCKETS 13

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct
{
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf hashbucket[NBUCKETS]; //每个哈希队列一个linked list 及一个lock
} bcache;

void binit(void)
{
  struct buf *b;

  //为每一个哈希桶分配一个锁
  for (int i = 0; i < NBUCKETS; i++)
  {
    initlock(&bcache.lock[i], "bcache");
    // Create linked list of buffers
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }

  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->next = bcache.hashbucket[0].next; //全给第一个  其他的从第一个偷
    b->prev = &bcache.hashbucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
  }
}

// 哈希表，将进程随机打散对应到某个哈希桶里
int Hash(uint blockno)
{
  return blockno % NBUCKETS;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;
  int b_now = Hash(blockno); //要查找，先用哈希表打散，找到对应的哈希桶。

  acquire(&bcache.lock[b_now]);

  // Is the block already cached?
  for (b = bcache.hashbucket[b_now].next; b != &bcache.hashbucket[b_now]; b = b->next)
  {
    //cached.
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.lock[b_now]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  int b_next = (b_now + 1) % NBUCKETS; //按顺序查找未被使用的数据缓存块

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 当走完13个桶，又要回到最初的桶，则说明数据块在所有桶都未命中
  // 为什么一个往前查一个往后查？
  while (b_next != b_now)
  {
    acquire(&bcache.lock[b_next]);
    for (b = bcache.hashbucket[b_next].prev; b != &bcache.hashbucket[b_next]; b = b->prev)
    {
      if (b->refcnt == 0) //命中
      {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        // 要先将它脱出!!! //
        b->next->prev = b->prev;
        b->prev->next = b->next;
        // 窃取缓存块(双向链表) //
        b->next = bcache.hashbucket[b_now].next;
        b->prev = &bcache.hashbucket[b_now];
        release(&bcache.lock[b_next]);
        bcache.hashbucket[b_now].next->prev = b;
        bcache.hashbucket[b_now].next = b;
        release(&bcache.lock[b_now]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[b_next]);
    b_next = (b_next + 1) % NBUCKETS;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
  int b_now = Hash(b->blockno); //要查找，先用哈希表打散，找到对应的哈希桶。

  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock[b_now]);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[b_now].next;
    b->prev = &bcache.hashbucket[b_now];
    bcache.hashbucket[b_now].next->prev = b;
    bcache.hashbucket[b_now].next = b;
  }

  release(&bcache.lock[b_now]);
}

void bpin(struct buf *b)
{
  int b_now = Hash(b->blockno); //要查找，先用哈希表打散，找到对应的哈希桶。

  acquire(&bcache.lock[b_now]);
  b->refcnt++;
  release(&bcache.lock[b_now]);
}

void bunpin(struct buf *b)
{
  int b_now = Hash(b->blockno); //要查找，先用哈希表打散，找到对应的哈希桶。

  acquire(&bcache.lock[b_now]);
  b->refcnt--;
  release(&bcache.lock[b_now]);
}
