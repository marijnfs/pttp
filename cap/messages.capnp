@0xe6ca35b9af8189a0;

struct RegularCredit {
       pub @0 :Text;
}

struct RelTimeLockedCredit {
       pub @0 :Text;
       time @1 :UInt64;
}

struct Account {
       version @0 :UInt32;
       data @1 :Text;
}


struct Witness {
       type @0 :UInt32;
       data @1 :Text;
}

struct Transaction {
       struct Credit {
       	      account @0 :Text;
      	      amount @1 :UInt64;
       }
       credits @0 :List(Credit);
       signatures @1 :List(Witness); #only verify negative credits, can sign the full list or only itself
       transactionId @2 :Text;
       bla @3 :List(Text);
}

struct Block {
       hash @0 :Text;
       transactionHash @1 :Text;
       utxoHash @2 :Text;
       prevHash @3 :Text;
       salt @4 :Text;
       time @5 :UInt64;
}

struct TransactionSet {
      transactions @0 :List(Transaction);
}


struct Hello {
       pub @0 :Text;
       port @1 :Int16;
}

struct Welcome {
       pub @0 :Text;
}

struct IpList {
       ips @0 :List(Text);
}
