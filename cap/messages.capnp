@0xe6ca35b9af8189a0;

struct RegularCredit {
       pub @0 :Text;
}

struct RelTimeLockedCredit {
       pub @0 :Data;
       time @1 :UInt64;
}

struct Account {
       version @0 :UInt32;
       data @1 :Data;
}


struct Witness {
       type @0 :UInt32; #for now refers to credit set signed, 0 = first, 1 = first+second, etc. To allow for fee increases etc.
       data @1 :Data;
}

struct Credit {
       account @0 :Data;
       amount @1 :UInt64;
}

struct CreditSet {
       credits @0 :List(Credit);
}

struct Transaction {
       #credits @0 :List(Credit);
       creditSets @0 :List(Data); #serialised credit sets so they can be signed
       signatures @1 :List(Witness); #only verify negative credits, can sign the full list or only itself
       transactionId @2 :Data;
}

struct Block {
       hash @0 :Data;
       transactionHash @1 :Data;
       utxoHash @2 :Data;
       prevHash @3 :Data;
       salt @4 :Data;
       time @5 :UInt64;
}

struct TransactionSet {
      transactions @0 :List(Transaction);
}


struct Hello {
       pub @0 :Data;
       port @1 :Int16;
}

struct Welcome {
       pub @0 :Data;
}

struct IpList {
       ips @0 :List(Data);
}

struct Message {
       content :union {
       	       hello @0 :Hello;
	       welcome @1 :Welcome;
	       iplist @2 :IpList;
       }
}
