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
       creditSet @0 :Data; #serialised credit sets so they can be signed
       signatures @1 :List(Witness); #only verify negative credits, can sign the full list or only itself
       hash @2 :Data; #blake hash of creditsets
}

struct Block {
       prevHash @0 :Data;
       transactionHash @1 :Data;
       utxoHash @2 :Data;
       nonce @3 :Data;
       time @4 :UInt64;
}

struct TransactionSet {
      transactions @0 :List(Transaction);
}

struct Hello {
       ip @0 :Text;
       port @1 :Int16;
}

struct Welcome {
       pub @0 :Data;
}

struct OK {
       message @0 :Text;
}

struct Reject {
       message @0 :Text;
}

struct GetPeers {

}

struct IpList {
       ips @0 :List(Text);
}

struct Message {
       content :union {
       	       hello @0 :Hello;
	       welcome @1 :Welcome;
	       getPeers @2 :GetPeers;
	       ipList @3 :IpList;
	       ok @4 :OK;
	       reject @5 :Reject;
	       transaction @6 :Transaction;
	       block @7 :Block;
       }
}
