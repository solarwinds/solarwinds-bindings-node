/* global describe, it */
'use strict'

const bindings = require('../')
const expect = require('chai').expect

const Sanitizer = bindings.Sanitizer

describe('bindings.Sanitizer', function () {
  it('should sanitizes an insert list', function () {
    const sql = "INSERT INTO `queries` (`asdf_id`, `asdf_prices`, `created_at`, `updated_at`, `blue_pill`, `yearly_tax`, `rate`, `steam_id`, `red_pill`, `dimitri`, `origin`) VALUES (19231, 3, 'cat', 'dog', 111.0, 126.0, 116.0, 79.0, 72.0, 73.0, ?, 1, 3, 229.284, ?, ?, 100, ?, 0, 3, 1, ?, NULL, NULL, ?, 4, ?)"
    const result = Sanitizer.sanitize(sql)
    expect(result).equal("INSERT INTO `queries` (`asdf_id`, `asdf_prices`, `created_at`, `updated_at`, `blue_pill`, `yearly_tax`, `rate`, `steam_id`, `red_pill`, `dimitri`, `origin`) VALUES (0, 0, '?', '?', 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, ?, 0, 0, 0.0, ?, ?, 0, ?, 0, 0, 0, ?, NULL, NULL, ?, 0, ?)")
  })

  it('should sanitizes a in list', function () {
    const sql = 'SELECT "game_types".* FROM "game_types" WHERE "game_types"."game_id" IN (1162)'
    const result = Sanitizer.sanitize(sql)
    expect(result).equal('SELECT "?".* FROM "?" WHERE "?"."?" IN (0)')
  })

  it('should sanitizes args in string', function () {
    const sql = "SELECT \"comments\".* FROM \"comments\" WHERE \"comments\".\"commentable_id\" = 2798 AND \"comments\".\"commentable_type\" = 'Video' AND \"comments\".\"parent_id\" IS NULL ORDER BY comments.created_at DESC"
    const result = Sanitizer.sanitize(sql)
    expect(result).equal("SELECT \"?\".* FROM \"?\" WHERE \"?\".\"?\" = 0 AND \"?\".\"?\" = '?' AND \"?\".\"?\" IS NULL ORDER BY comments.created_at DESC")
  })

  it('should sanitizes a in list with option to keep double quoted values', function () {
    const sql = 'SELECT "game_types".* FROM "game_types" WHERE "game_types"."game_id" IN (1162)'
    const result = Sanitizer.sanitize(sql, Sanitizer.OBOE_SQLSANITIZE_KEEPDOUBLE)
    expect(result).equal('SELECT "game_types".* FROM "game_types" WHERE "game_types"."game_id" IN (0)')
  })

  it('should sanitizes args in string with option to keep double quoted values', function () {
    const sql = "SELECT \"comments\".* FROM \"comments\" WHERE \"comments\".\"commentable_id\" = 2798 AND \"comments\".\"commentable_type\" = 'Video' AND \"comments\".\"parent_id\" IS NULL ORDER BY comments.created_at DESC"
    const result = Sanitizer.sanitize(sql, Sanitizer.OBOE_SQLSANITIZE_KEEPDOUBLE)
    expect(result).equal("SELECT \"comments\".* FROM \"comments\" WHERE \"comments\".\"commentable_id\" = 0 AND \"comments\".\"commentable_type\" = '?' AND \"comments\".\"parent_id\" IS NULL ORDER BY comments.created_at DESC")
  })

  it('should sanitizes a mixture of situations', function () {
    const sql = "SELECT `assets`.* FROM `assets` WHERE `assets`.`type` IN ('Picture') AND (updated_at >= '2015-07-08 19:22:00') AND (updated_at <= '2015-07-08 19:23:00') LIMIT 31 OFFSET 0"
    const result = Sanitizer.sanitize(sql)
    expect(result).equal("SELECT `assets`.* FROM `assets` WHERE `assets`.`type` IN ('?') AND (updated_at >= '?') AND (updated_at <= '?') LIMIT 0 OFFSET 0")
  })

  it('should sanitizes quoted stuff', function () {
    const sql = "SELECT `users`.* FROM `users` WHERE (mobile IN ('234 234 234') AND email IN ('a_b_c@hotmail.co.uk'))"
    const result = Sanitizer.sanitize(sql)
    expect(result).equal("SELECT `users`.* FROM `users` WHERE (mobile IN ('?') AND email IN ('?'))")
  })

  it('should sanitizes complicated quoted stuff', function () {
    const sql = "SELECT `users`.* FROM `users` WHERE (mobile IN ('2342423') AND email IN ('a_b_c@hotmail.co.uk')) LIMIT 5"
    const result = Sanitizer.sanitize(sql)
    expect(result).equal("SELECT `users`.* FROM `users` WHERE (mobile IN ('?') AND email IN ('?')) LIMIT 0")
  })

  it('should not remove numbers from column names \'', function () {
    const sql = 'UPDATE my_table SET col1 = 10, col2 = 20, col3 = 30 WHERE col1 = 1'
    const result = Sanitizer.sanitize(sql)
    expect(result).equal('UPDATE my_table SET col1 = 0, col2 = 0, col3 = 0 WHERE col1 = 0')
  })

  // Note the FSM (as copied from the PHP agent) can NOT handle an escaped value correctly.
  it('would NOT properly handle escaped \'', function () {
    const sql = "SELECT * FROM test_table tbl1 WHERE tbl1.name = 'jake\'s' GROUP BY tbl1.name"
    const result = Sanitizer.sanitize(sql)
    // below not really what we would expect.
    // we would expect: SELECT * FROM test_table tbl1 WHERE tbl1.name = ? GROUP BY tbl1.name
    // however the sanitizer can't handle that. No fix will be implemented as current agent is EOL 
    // and future version of the agent will use a regex as in the ruby agent.
    expect(result).equal("SELECT * FROM test_table tbl1 WHERE tbl1.name = '?'s'?")
  })
})
