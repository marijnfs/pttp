@0x9786a5ccc9815e0c;

struct RegularCredit {
       pub @0 :Text;
}

struct RelTimeLockedCredit {
       pub @0 :Text;
       time @1 :Date;
}

struct Credit {
       version @0 :UInt32;
       data @1 :Text;
       amount @2 :UInt64;
}

struct Transaction {
       credits @0 :List(Credit);
       signatures @1 :List(Text); //only verify negative credits, have to sign the full list
       transaction_id :Text;
}

struct Block {
       hash @0 :Text;
       transaction_hash @1 :Text;
       utxo_hash @2 :Text;
       prev_hash @3 :Text;
       salt @4 :Text;
       time @5 :Date;
}

struct Transactions {
       transactions @0 :List(Transaction);
}


struct Hello {
       pub @0 :Text;
       port @1 :Int16;
}

struct Welcome {
       pub @0 :Text;
}

struct List {
       ips @0 :Text;
}

