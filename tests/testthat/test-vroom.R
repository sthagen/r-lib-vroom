context("test-vroom.R")

test_vroom <- function(content, ..., equals) {
  expect_equivalent(
    vroom(content, ...),
    equals
  )

  tf <- tempfile()
  on.exit(unlink(tf))
  readr::write_lines(content, tf)

  con <- file(tf, "rb")
  on.exit(close(con), add = TRUE)

  res <- vroom(con, ...)

  # Has a temp_file environment, with a filename
  tf2 <- attr(res, "filename")
  expect_true(is.character(tf2))
  expect_true(file.exists(tf2))
  expect_equivalent(res, equals)

  rm(res)
  gc()

  # Which is removed after the object is deleted and the finalizer has run
  expect_false(file.exists(tf2))
}

test_that("vroom can read a tsv", {
  test_vroom("a\tb\tc\n1\t2\t3\n",
    equals = tibble::tibble(a = 1, b = 2, c = 3)
  )
})

test_that("vroom can read a csv", {
  test_vroom("a,b,c\n1,2,3\n", delim = ",",
    equals = tibble::tibble(a = 1, b = 2, c = 3)
  )
})

test_that("vroom guesses columns with NAs", {
  test_vroom("a,b,c\nNA,2,3\n4,5,6", delim = ",",
    equals = tibble::tibble(a = c(NA, 4), b = c(2, 5), c = c(3, 6))
  )

  test_vroom("a,b,c\nfoo,2,3\n4,5,6", delim = ",", na = "foo",
    equals = tibble::tibble(a = c(NA, 4), b = c(2, 5), c = c(3, 6))
  )

  test_vroom("a,b,c\nfoo,2,3\n4.0,5,6", delim = ",", na = "foo",
    equals = tibble::tibble(a = c(NA, 4), b = c(2, 5), c = c(3, 6))
  )

  test_vroom("a,b,c\nfoo,2,3\nbar,5,6", delim = ",", na = "foo",
    equals = tibble::tibble(a = c(NA, "bar"), b = c(2, 5), c = c(3, 6))
  )
})

test_that("vroom can read files with quotes", {
  test_vroom('"a","b","c"\n"foo","bar","baz"', delim = ",",
    equals = tibble::tibble(a = "foo", b = "bar", c = "baz")
  )

  test_vroom('"a","b","c"\n",foo","bar\n",""', delim = ",", na = character(),
    equals = tibble::tibble(a = ",foo", b = "bar\n", c = "")
  )

  test_vroom('"a","b","c"\n",foo","bar\n",""', delim = ",", na = character(),
    equals = tibble::tibble(a = ",foo", b = "bar\n", c = "")
  )
})
