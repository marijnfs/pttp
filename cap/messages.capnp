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
       type @0 :UInt32; #for now refers to credit set signed, 0 = first, 1 = first+second, etc. To allow for fee increases etc.
       data @1 :Text;
}

struct Credit {
       account @0 :Text;
       amount @1 :UInt64;
}

struct CreditSet {
       credits @0 :List(Credit);
}

struct Transaction {
       #credits @0 :List(Credit);
       creditSets @0 :List(Text); #serialised credit sets so they can be signed
       signatures @1 :List(Witness); #only verify negative credits, can sign the full list or only itself
       transactionId @2 :Text;
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

struct Message {
       content :union {
       	       hello @0 :Hello;
	       welcome @1 :Welcome;
	       iplist @2 :IpList;
       }
}
