$1=="#" {
  next
}

$1=="splot" || $1=="replot" {
  print "#BEFORE_PLOT"
  print
  print "#AFTER_PLOT"
  next
}

$1=="pause" {
  print "#BEFORE_PAUSE"
  print
  next
}

{ print }
