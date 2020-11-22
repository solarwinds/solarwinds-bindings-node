#! /usr/bin/env node

// auth options https://developer.github.com/v3/auth/
// trigger https://docs.github.com/en/free-pro-team@latest/rest/reference/actions#create-a-workflow-dispatch-event
//
// curl -X POST -H "Accept: application/vnd.github.v3+json" -H "Authorization: token $GIT_TOKEN"  \
//   https://api.github.com/repos/appoptics/appoptics-bindings-node/actions/workflows/bindings-os-tests.yml/dispatches   -d '{"ref":"github-actions"}'
//
// what a pain. this first must be published with a on: push (or other active)
// trigger before it will show up in actions OR be able to be triggered via
// the POST shown above. what is github thinking? and apparently it cannot be trigger
// using the webpage button unless the workflow is in the master branch.
//
// delete a workflow run https://docs.github.com/en/free-pro-team@latest/rest/reference/actions#delete-a-workflow-run
// curl -X DELETE -H "Accept: application/vnd.github.v3+json" -H "Authorization: token $GIT_TOKEN" \
//   https://api.github.com/repos/appoptics/appoptics-bindings-node/actions/runs/363620271

const axios = require('axios');

// simple syntax ./gh-actions.js workflow-name workflow-action workflow-specific-arguments
// workflow-action: list-runs


const [wfName, action, ...args] = process.argv.slice(2);

const actions = {
  'list-runs': listRuns,
  'get-run': getRun,
  'delete-run': deleteRun,
  'initiate': initiate,
};


//
// actions
//

const apiRoot = 'https://api.github.com';
const owner = 'appoptics';
const repo = 'appoptics-bindings-node';

const headers = {
  accept: 'application/vnd.github.v3+json',
  authorization: `token ${process.env.GIT_TOKEN}`,
};

// might need to iterate due to per_page limit of 100
// https://docs.github.com/en/free-pro-team@latest/rest/reference/actions#list-workflow-runs
async function listRuns () {
  const url = `${apiRoot}/repos/${owner}/${repo}/actions/workflows/${wfName}/runs?per_page=100`;
  return axios.get(url, {headers})
    .then(r => {
      const results = {count: 0, workflow_runs: []};
      const o = results.workflow_runs;
      for (const wfr of r.data.workflow_runs) {
        o.push({run_number: wfr.run_number, id: wfr.id});
      }
      results.count = o.length;
      return results;
    });
}

//
//curl -X DELETE -H "Accept: application/vnd.github.v3+json" -H "Authorization: token $GIT_TOKEN" \
//  https://api.github.com/repos/appoptics/appoptics-bindings-node/actions/runs/363620271
//
async function deleteRun (args) {
  const range = makeRange(...args);
  const {count, workflow_runs: runs} = await listRuns();
  const runToIdMap = new Map();
  for (let i = 0; i < count; i++) {
    runToIdMap.set(runs[i].run_number, runs[i].id);
  }
  const p = [];
  const ids = [];
  const coreUrl = `${apiRoot}/repos/${owner}/${repo}/actions/runs`;
  for (const i of range) {
    if (runToIdMap.has(i)) {
      const url = `${coreUrl}/${runToIdMap.get(i)}`;
      ids.push(i);
      p.push(axios.delete(url, {headers}));
    }
  }
  return Promise.allSettled(p)
    .then(r => {
      return r.map((p, ix) => {
        if (p.status === 'rejected') {
          return {id: ids[ix], error: p.reason.message};
        }
        return {id: ids[ix], status: p.value.status};
      });
    });
}

// GET /repos/:owner/:repo/actions/runs/:run_id
async function getRun (args) {
  const range = makeRange(...args);
  const {count, workflow_runs: runs} = await listRuns();
  const runToIdMap = new Map();
  for (let i = 0; i < count; i++) {
    runToIdMap.set(runs[i].run_number, runs[i].id);
  }
  const p = [];
  const coreUrl = `${apiRoot}/repos/${owner}/${repo}/actions/runs`;
  for (const i of range) {
    if (runToIdMap.has(i)) {
      const url = `${coreUrl}/${runToIdMap.get(i)}`;
      p.push(axios.get(url, {headers}).then(r => r.data));
    }
  }
  return Promise.allSettled(p)
    .then(r => {
      return r.map(p => {
        if (p.status === 'rejected') {
          return p.reason.message;
        }
        return p.value;
      });
    });
}

// curl -X POST -H "Accept: application/vnd.github.v3+json" -H "Authorization: token $GIT_TOKEN" \
//   https://api.github.com/repos/appoptics/appoptics-bindings-node/actions/workflows/bindings-os-tests.yml/dispatches -d '{"ref":"github-actions"}'
//
async function initiate (args) {
  const url = `${apiRoot}/repos/${owner}/${repo}/actions/workflows/${wfName}/dispatches`;
  const data = {ref: args[0] || 'github-actions'};
  return axios.post(url, data, {headers})
    .then(r => {
      return {status: r.status};
    });
}

function makeRange (low, high) {
  let lo, hi;
  if (!low || !(lo = Number(low))) {
    throw new Error(`invalid value for range ${low}`);
  }
  const range = [lo];
  if (high === undefined) {
    return range;
  }
  if (!high || !(hi = Number(high))) {
    throw new Error(`invalid value for range ${high}`);
  }
  for (let i = lo + 1; i <= hi; i++) {
    range.push(i);
  }
  return range;
}

/* eslint-disable no-console */
(function main () {
  if (action in actions) {
    actions[action](args)
      .then(r => {
        console.log(r);
      })
      .catch(e => {
        console.error(`error executing ${action}: ${e.message}`);
        if (e.response && e.response.data) {
          console.error(`  response message: ${e.response.data.message}`);
        }
        process.exit(1);
      });
  } else {
    console.error(`invalid action: ${action}`);
    process.exit(1);
  }
})();


