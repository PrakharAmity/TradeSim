const byId = (id) => document.getElementById(id);
const money = new Intl.NumberFormat("en-US", { style: "currency", currency: "USD", maximumFractionDigits: 0 });
let appState = null;
let selectedResults = [];

async function getJson(url, options = {}) {
  const response = await fetch(url, { headers: { "Content-Type": "application/json" }, ...options });
  if (!response.ok) throw new Error(`Request failed (${response.status})`);
  return response.json();
}

function setNotice(message, visible = true) {
  const notice = byId("notice");
  notice.textContent = message;
  notice.classList.toggle("hidden", !visible);
}

function populateControls(state) {
  const ticker = byId("ticker-select");
  ticker.innerHTML = state.markets.map((market) => `<option value="${market.ticker}">${market.name}</option>`).join("");
  const strategy = byId("strategy-select");
  strategy.innerHTML = '<option value="all">Strategy Comparison (All)</option>' +
    state.strategies.map((item) => `<option value="${item.id}">${item.name}</option>`).join("");
}

function renderChart(prices, ticker) {
  byId("chart-title").textContent = `${ticker} · Daily closing price`;
  byId("session-count").textContent = `${prices.length} sessions`;
  document.querySelector(".chart-period").textContent = `${prices.length} SESSIONS`;
  const svg = byId("price-chart");
  if (!prices.length) { svg.innerHTML = ""; byId("chart-ylabels").innerHTML = ""; byId("chart-xlabels").innerHTML = ""; return; }
  const min = Math.min(...prices), max = Math.max(...prices);
  const range = Math.max(max - min, 1);
  const padMin = min - range * 0.12, padMax = max + range * 0.12;
  const points = prices.map((price, index) => {
    const x = prices.length === 1 ? 480 : 8 + index * 944 / (prices.length - 1);
    const y = 207 - (price - padMin) * 194 / (padMax - padMin);
    return [x, y];
  });
  const line = points.map(([x, y], index) => `${index ? "L" : "M"}${x.toFixed(1)} ${y.toFixed(1)}`).join(" ");
  const area = `${line} L${points[points.length - 1][0].toFixed(1)} 214 L${points[0][0].toFixed(1)} 214 Z`;
  const last = points[points.length - 1];
  svg.innerHTML = `<defs><linearGradient id="price-gradient" x1="0" x2="0" y1="0" y2="1"><stop offset="0" stop-color="#70d4a9" stop-opacity=".22"/><stop offset="1" stop-color="#70d4a9" stop-opacity="0"/></linearGradient></defs><path class="price-area" d="${area}"/><path class="price-line" d="${line}"/><circle class="price-dot" cx="${last[0]}" cy="${last[1]}" r="4"/>`;
  byId("chart-ylabels").innerHTML = [1, .66, .33, 0].map((part) => `<span>${money.format(padMin + part * (padMax - padMin))}</span>`).join("");
  const labels = prices.map((_, index) => `D${index + 1}`);
  const maxLabels = Math.min(labels.length, 6);
  const xLabels = Array.from({ length: maxLabels }, (_, i) => labels[Math.round(i * (labels.length - 1) / Math.max(maxLabels - 1, 1))]);
  byId("chart-xlabels").innerHTML = xLabels.map((label) => `<span>${label}</span>`).join("");
}

function renderComparison(results) {
  byId("strategy-rows").innerHTML = results.map((result) => `
    <tr>
      <td><span class="strategy-name">${result.strategy}</span><span class="strategy-sub">${result.transactions} completed cycle${result.transactions === 1 ? "" : "s"}</span></td>
      <td class="money ${result.totalProfit >= 0 ? "positive" : "negative"}">${money.format(result.totalProfit)}</td>
      <td>${result.transactions}</td>
      <td class="money">${money.format(result.maxDrawdown)}</td>
      <td><span class="status-pill ${result.valid ? "good" : "bad"}"><i></i>${result.valid ? "✓ Valid Strategy" : "⚠ Unexpected Result"}</span></td>
    </tr>`).join("");

  const returns = results.map((result) => result.totalProfit);
  const drawdowns = results.map((result) => result.maxDrawdown);
  const anomalies = results.filter((result) => !result.valid).length;
  byId("metric-profit").textContent = money.format(Math.max(0, ...returns));
  byId("metric-profit-note").textContent = `Highest return across ${results.length} selected polic${results.length === 1 ? "y" : "ies"}`;
  byId("metric-trades").textContent = results.reduce((sum, result) => sum + result.transactions, 0).toLocaleString("en-US");
  byId("metric-trades-note").textContent = `Across ${results.length} selected polic${results.length === 1 ? "y" : "ies"}`;
  byId("metric-drawdown").textContent = money.format(Math.max(0, ...drawdowns));
  byId("metric-drawdown-note").textContent = "Largest observed market pullback";
  byId("metric-health").textContent = anomalies ? `${anomalies} anomaly flag${anomalies === 1 ? "" : "s"}` : "All policies valid";
  byId("metric-health").className = `metric-value health-value ${anomalies ? "unexpected" : "healthy"}`;
  byId("metric-health-note").textContent = anomalies ? "Review execution traces below" : "No policy invariants breached";
  if (anomalies) setNotice(`${anomalies} selected strateg${anomalies === 1 ? "y reports" : "ies report"} an unexpected result. Inspect the execution trace and compare the policy rules.`, true);
  else setNotice("Backtest complete. All selected policies satisfy the execution checks.", true);
}

function renderTransactions(results) {
  const events = [];
  for (const result of results) for (const trade of result.trades) events.push({ ...trade, strategy: result.strategy });
  events.sort((a, b) => a.day - b.day || a.strategy.localeCompare(b.strategy));
  byId("transaction-count").textContent = `${events.length} EVENTS`;
  byId("transaction-rows").innerHTML = events.length ? events.map((trade) => `
    <tr>
      <td class="strategy-name">${trade.strategy}</td>
      <td><span class="action-pill ${trade.action.toLowerCase()}">${trade.action}</span></td>
      <td class="session-cell">DAY ${trade.day + 1}</td>
      <td class="price-cell">${money.format(trade.price)}</td>
      <td class="money ${trade.action === "SELL" && trade.realizedProfit >= 0 ? "positive" : trade.realizedProfit < 0 ? "negative" : ""}">${trade.action === "SELL" ? money.format(trade.realizedProfit) : "—"}</td>
    </tr>`).join("") : '<tr><td colspan="5" class="empty-cell">No completed transactions in this market period.</td></tr>';
}

async function runBacktest() {
  const button = byId("run-button");
  button.disabled = true;
  button.innerHTML = "◌ Running…";
  try {
    const payload = { ticker: byId("ticker-select").value, strategy: byId("strategy-select").value };
    const result = await getJson("/api/backtest", { method: "POST", body: JSON.stringify(payload) });
    selectedResults = result.results;
    renderChart(result.prices, result.ticker);
    renderComparison(result.results);
    renderTransactions(result.results);
    byId("engine-status").textContent = `Feed active · ${result.ticker}`;
  } catch (error) {
    byId("engine-status").textContent = "Request failed";
    setNotice(`Unable to run backtest: ${error.message}`, true);
  } finally {
    button.disabled = false;
    button.innerHTML = "<span>▶</span> Run backtest";
  }
}

async function resetScenario() {
  const button = byId("reset-button");
  button.disabled = true;
  try {
    await getJson("/api/reset", { method: "POST", body: "{}" });
    byId("ticker-select").value = "ACME_TECH";
    byId("strategy-select").value = "all";
    await runBacktest();
  } catch (error) {
    setNotice(`Unable to reset the scenario: ${error.message}`, true);
  } finally {
    button.disabled = false;
  }
}

async function boot() {
  try {
    appState = await getJson("/api/state");
    populateControls(appState);
    byId("engine-status").textContent = "Feed active · ACME_TECH";
    byId("ticker-select").addEventListener("change", runBacktest);
    byId("strategy-select").addEventListener("change", runBacktest);
    byId("run-button").addEventListener("click", runBacktest);
    byId("reset-button").addEventListener("click", resetScenario);
    await runBacktest();
  } catch (error) {
    byId("engine-status").textContent = "Engine unavailable";
    setNotice(`Could not connect to the TradeSim C++ service. Start it with ./start.sh. ${error.message}`, true);
  }
}

boot();
