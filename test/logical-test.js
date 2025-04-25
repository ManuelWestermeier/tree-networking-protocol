// === Core logic ===

// Compute how many leading elements match, and how many remain unmatched
function match(connection, pocket) {
  const minLen = Math.min(connection.length, pocket.length);
  let positive = 0;

  // Count equal elements from the front
  while (positive < minLen && connection[positive] === pocket[positive]) {
    positive++;
  }

  // Negative = how many in the connection are past the last match
  const negative = connection.length - positive;

  return { positive, negative };
}

// Compute a single “score” from a Match object
function matchIndex(m) {
  return m.positive - m.negative;
}

// Deep-equality for two arrays of the same ints
function eq(a1, a2) {
  if (a1.length !== a2.length) return false;
  for (let i = 0; i < a1.length; i++) {
    if (a1[i] !== a2[i]) return false;
  }
  return true;
}

class Connection {
  constructor(address, pin) {
    this.address = address; // array of ints
    this.pin = pin; // number
  }
}

class Node {
  constructor(connections = [], you = []) {
    this.connections = connections; // array of Connection
    this.you = you; // your own Address (array of ints)
  }

  // Pick the connection whose address “best matches” the pocket’s address
  send(pocket) {
    if (this.connections.length === 0) {
      console.log("No available connections to send data.");
      return 0;
    }

    // Start with the first connection as “best”
    let bestConn = this.connections[0];
    let bestScore = matchIndex(match(bestConn.address, pocket.address));

    // See if any other connection beats it
    for (let i = 1; i < this.connections.length; i++) {
      const conn = this.connections[i];
      const score = matchIndex(match(conn.address, pocket.address));
      if (score > bestScore) {
        bestScore = score;
        bestConn = conn;
      }
    }

    console.log(`Sending data via pin ${bestConn.pin}`);
    return bestConn.pin;
  }

  // If the pocket is addressed to you, consume it (return 0),
  // otherwise forward it along the best route
  receive(pocket) {
    if (eq(this.you, pocket.address)) {
      console.log("Pocket is for me!");
      return 0;
    } else {
      return this.send(pocket);
    }
  }
}

// === Example tests ===

// Define a couple of connections
const connA = new Connection([1, 2, 3], 10);
const connB = new Connection([1, 2, 4], 11);
const connC = new Connection([1, 3], 12);

// Build a node whose “you” address is [1,2,3]
const node = new Node([connA, connB, connC], [1, 2, 3]);

// Test 1: pocket addressed exactly to me
const pocket1 = { address: [1, 2, 3] };
console.log("Test 1 returned pin:", node.receive(pocket1));
// → “Pocket is for me!” and returns 0

// Test 2: pocket [1,2,5] – both connA & connB share 2 leading matches then diverge,
//                 but they each have 1 leftover, so tie.
//                 we keep connA (first) → pin 10
const pocket2 = { address: [1, 2, 5] };
console.log("Test 2 returned pin:", node.receive(pocket2));
// → “Sending data via pin 10” and returns 10

// Test 3: pocket [1,2,4] – connB matches all 3 of its components (positive=3, negative=0 → score=3),
//                          connA only matches first 2 (2−1=1), connC only matches 1 (1−1=0). 
//                          so we pick connB → pin 11
const pocket3 = { address: [1, 2, 4] };
console.log("Test 3 returned pin:", node.receive(pocket3));
// → “Sending data via pin 11” and returns 11

// Test 4: pocket [1,3,9] – connC matches at [1], then diverges (1−1=0) vs connA/B (1−2=−1),
//                          so we pick connC → pin 12
const pocket4 = { address: [1, 3, 9] };
console.log("Test 4 returned pin:", node.receive(pocket4));
// → “Sending data via pin 12” and returns 12
