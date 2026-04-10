const http = require('http');

const TARGET_URL = 'http://localhost:3006/post/run';
const PAYLOAD = JSON.stringify(
    {"cmd":"get_global_json","args":{"token":"jd"}}
);
const NUM_REQUESTS = 10000;

class BenchmarkStats {
  constructor() {
    this.times = [];
    this.errors = 0;
    this.successes = 0;
  }

  addTime(ms) {
    this.times.push(ms);
    this.successes++;
  }

  addError() {
    this.errors++;
  }

  getStats() {
    if (this.times.length === 0) {
      return {
        min: 0,
        max: 0,
        avg: 0,
        median: 0,
        p95: 0,
        p99: 0,
        successes: this.successes,
        errors: this.errors,
        total: this.successes + this.errors
      };
    }

    const sorted = this.times.slice().sort((a, b) => a - b);
    const sum = sorted.reduce((a, b) => a + b, 0);

    return {
      min: sorted[0],
      max: sorted[sorted.length - 1],
      avg: sum / sorted.length,
      median: sorted[Math.floor(sorted.length / 2)],
      p95: sorted[Math.floor(sorted.length * 0.95)],
      p99: sorted[Math.floor(sorted.length * 0.99)],
      successes: this.successes,
      errors: this.errors,
      total: this.successes + this.errors
    };
  }
}

function makeRequest() {
  return new Promise((resolve, reject) => {
    const startTime = Date.now();

    const options = {
      hostname: 'localhost',
      port: 3006,
      path: '/post/run',
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(PAYLOAD)
      }
    };

    const req = http.request(options, (res) => {
      let data = '';

      res.on('data', (chunk) => {
        data += chunk;
      });

      res.on('end', () => {
        const endTime = Date.now();
        const duration = endTime - startTime;

        if (res.statusCode >= 200 && res.statusCode < 300) {
          resolve({ success: true, duration, statusCode: res.statusCode });
        } else {
          resolve({ success: false, duration, statusCode: res.statusCode, error: `HTTP ${res.statusCode}` });
        }
      });
    });

    req.on('error', (error) => {
      const endTime = Date.now();
      const duration = endTime - startTime;
      resolve({ success: false, duration, error: error.message });
    });

    req.write(PAYLOAD);
    req.end();
  });
}

async function runBenchmark() {
  console.log(`\n========================================`);
  console.log(`Benchmark Test: ${TARGET_URL}`);
  console.log(`========================================`);
  console.log(`Payload: ${PAYLOAD}`);
  console.log(`Number of requests: ${NUM_REQUESTS}`);
  console.log(`Starting benchmark...\n`);

  const stats = new BenchmarkStats();
  const startTime = Date.now();

  // Progress indicator
  const progressInterval = Math.floor(NUM_REQUESTS / 10);

  for (let i = 0; i < NUM_REQUESTS; i++) {
    const result = await makeRequest();

    if (result.success) {
      stats.addTime(result.duration);
    } else {
      stats.addError();
      if (i < 5) { // Only log first few errors to avoid spam
        console.error(`Error on request ${i + 1}: ${result.error || 'Unknown error'}`);
      }
    }

    if ((i + 1) % progressInterval === 0) {
      process.stdout.write(`Progress: ${i + 1}/${NUM_REQUESTS} (${Math.floor((i + 1) / NUM_REQUESTS * 100)}%)\r`);
    }
  }

  const totalTime = Date.now() - startTime;
  const benchStats = stats.getStats();

  // Print benchmark results
  console.log(`\n\n========================================`);
  console.log(`BENCHMARK RESULTS`);
  console.log(`========================================\n`);

  // Summary table
  console.log(`┌─────────────────────────────────────────┐`);
  console.log(`│          SUMMARY STATISTICS             │`);
  console.log(`├─────────────────────────┬───────────────┤`);
  console.log(`│ Total Requests          │ ${String(benchStats.total).padStart(13)} │`);
  console.log(`│ Successful              │ ${String(benchStats.successes).padStart(13)} │`);
  console.log(`│ Failed                  │ ${String(benchStats.errors).padStart(13)} │`);
  console.log(`│ Success Rate            │ ${String((benchStats.successes / benchStats.total * 100).toFixed(2) + '%').padStart(13)} │`);
  console.log(`│ Total Time              │ ${String(totalTime.toFixed(2) + ' ms').padStart(13)} │`);
  console.log(`│ Requests/sec            │ ${String((NUM_REQUESTS / (totalTime / 1000)).toFixed(2)).padStart(13)} │`);
  console.log(`└─────────────────────────┴───────────────┘`);

  console.log(`\n┌─────────────────────────────────────────┐`);
  console.log(`│       RESPONSE TIME (ms)                │`);
  console.log(`├─────────────────────────┬───────────────┤`);
  console.log(`│ Minimum                 │ ${String(benchStats.min.toFixed(2)).padStart(13)} │`);
  console.log(`│ Maximum                 │ ${String(benchStats.max.toFixed(2)).padStart(13)} │`);
  console.log(`│ Average                 │ ${String(benchStats.avg.toFixed(2)).padStart(13)} │`);
  console.log(`│ Median                  │ ${String(benchStats.median.toFixed(2)).padStart(13)} │`);
  console.log(`│ 95th Percentile         │ ${String(benchStats.p95.toFixed(2)).padStart(13)} │`);
  console.log(`│ 99th Percentile         │ ${String(benchStats.p99.toFixed(2)).padStart(13)} │`);
  console.log(`└─────────────────────────┴───────────────┘\n`);

  // Distribution table
  const buckets = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000];
  const distribution = new Array(buckets.length + 1).fill(0);

  stats.times.forEach(time => {
    let bucketIndex = buckets.findIndex(b => time < b);
    if (bucketIndex === -1) bucketIndex = buckets.length;
    distribution[bucketIndex]++;
  });

  console.log(`┌─────────────────────────────────────────┐`);
  console.log(`│     RESPONSE TIME DISTRIBUTION          │`);
  console.log(`├─────────────────────────┬───────────────┤`);
  console.log(`│ < 10ms                  │ ${String(distribution[0]).padStart(13)} │`);
  for (let i = 0; i < buckets.length - 1; i++) {
    console.log(`│ ${String(buckets[i] + '-' + (buckets[i + 1] - 1) + 'ms').padEnd(23)} │ ${String(distribution[i + 1]).padStart(13)} │`);
  }
  console.log(`│ >= ${buckets[buckets.length - 1]}ms${' '.repeat(19 - String(buckets[buckets.length - 1]).length)}│ ${String(distribution[buckets.length]).padStart(13)} │`);
  console.log(`└─────────────────────────┴───────────────┘\n`);

  console.log(`Test completed successfully!\n`);
}

// Run the benchmark
runBenchmark().catch(error => {
  console.error('Benchmark failed:', error);
  process.exit(1);
});
