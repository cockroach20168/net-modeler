// net-modeler.c
// Author: Chris Wacek (cwacek@cs.georgetown.edu)
//
// A netfilter based network emulator
//
#define AUTHOR "Chris Wacek <cwacek@cs.georgetown.edu>"

#include "version.i"
#include "nm_main.h"
#include "nm_magic.h"
#include "nm_structures.h"

static struct nf_hook_ops nfho;
struct iphdr *iph;

#define NM_IP_MASK 0x000000FF
#define NM_IP_NET 0x0000000A
#define IS_NM_IP(ip) (((ip) & NM_IP_MASK) == (NM_IP_NET))
#define NM_SEEN(iph) (iph->tos == TOS_MAGIC)

#define queue_entry_iph(x) ((struct iphdr *)skb_network_header((x)->skb))

unsigned int hook_func(unsigned int hooknum, struct sk_buff *skb,
                       const struct net_device *in,
                       const struct net_device *out,
                       int (*okfn)(struct sk_buff *))
{

  if (!skb) { return NF_ACCEPT; }
  iph = (struct iphdr *) skb_network_header(skb);

  if (! IS_NM_IP(iph->daddr) ){
    return NF_ACCEPT;
  }
  if (NM_SEEN(iph)){
    nm_log(NM_DEBUG,"Accepting previously seen packet. "IPH_FMT"\n",
                    IPH_FMT_DATA(iph));
    return NF_ACCEPT;
  }

  return NF_QUEUE;
}

ktime_t insert_delay(struct nm_global_sched * s){
  log_func_entry;

  return ktime_set(0,0);
}

ktime_t update(struct nm_global_sched *sch)
{
  struct nm_packet *pkt;
  int missed_intervals, i, dequeued_ctr = 0;
  ktime_t now = sch->timer.base->get_time();
  log_func_entry;
  
  nm_log(NM_DEBUG, "Callback fired at %lld\n",ktime_to_ns(now));

  /** Make sure we process any intervals we missed **/
  missed_intervals = ktime_to_ns(ktime_sub(now,sch->last_update)) 
                        / MSECS_TO_NSECS(UPDATE_INTERVAL_MSECS);

  for (i = 1; i <= missed_intervals; i++)
  {
    /** We dequeue everything in the slot one at at time **/
    while (( pkt = slot_pull(&scheduler_slot(sch,i))))
    {
      if (pkt->flags & NM_FLAG_HOP_INCOMPLETE) {
        /** If this was a hop in progress, we want to enqueue rather than
         * reinject **/
        /*nm_enqueue(pkt,path_dist(path_id,path_idx) - pkt->hop_progress);*/
      } else {
        dequeued_ctr++;
        nm_log(NM_DEBUG,"dequeued packet: "IPH_FMT"\n",
                    IPH_FMT_DATA(queue_entry_iph(pkt->data)));

        nf_reinject(pkt->data,NF_ACCEPT);
        nm_packet_free(pkt);
      }
    }
  }

  sch->now_index += missed_intervals;

  nm_log(NM_INFO, "Dequeued %u packets from %u intervals \n",dequeued_ctr,missed_intervals);

  return ktime_set(0,MSECS_TO_NSECS(UPDATE_INTERVAL_MSECS));
}

static int _nm_queue_cb(struct nf_queue_entry *entry, unsigned int queuenum)
{
  nm_packet_t *pkt; 
  int err;

  nm_log(NM_DEBUG,"Enqueuing new packet. "IPH_FMT"\n",
                    IPH_FMT_DATA(queue_entry_iph(entry)));

  pkt = nm_packet_init(entry,
                       queue_entry_iph(entry)->saddr, 
                       queue_entry_iph(entry)->daddr);
  
  if (!pkt)
    return -ENOMEM;
  
  if ((err = nm_enqueue(pkt,sizeof(nm_packet_t *))) < 0)
    return err;

  return 0;
}

static const struct nf_queue_handler _queueh = {
      .name   = "net-modeler",
      .outfn  = _nm_queue_cb,
};

static int __init nm_init(void)
{
  log_func_entry;
  printk(KERN_INFO "Starting up");

  nm_structures_init();

  check_call(nf_register_queue_handler(PF_INET,&_queueh));

  nfho.hook = hook_func;
  nfho.hooknum =  NF_INET_LOCAL_OUT;
  nfho.pf = PF_INET;
  nfho.priority = NF_IP_PRI_FIRST;

  nf_register_hook(&nfho);

  /* Initialize the global scheduler */
  if (nm_init_sched(update) < 0){
    nm_log(NM_WARN, "Failed to initialize scheduler\n");
    cleanup_module();
    return -1;
  }

  nm_log(NM_NOTICE,"net-modeler initialized ("VERSION")\n");

  nm_schedule(ktime_set(2,0));
  
  return 0;
}

static void nm_exit(void)
{
  nm_cleanup_sched();
  nm_cleanup_injector();
  nf_unregister_hook(&nfho);
  nf_unregister_queue_handler(PF_INET,&_queueh);
  nm_structures_release();
  printk(KERN_INFO "net-modeler cleaning up\n");
}

module_init(nm_init);
module_exit(nm_exit);

MODULE_AUTHOR(AUTHOR);
MODULE_LICENSE("GPL");
