(declare-const in1 String)
(declare-const in2 String)
(assert (< (str.len in1) 3))
(assert (< (str.len in2) 3))
(assert (str.in.re "abcd" (re.union (str.to.re in1) (str.to.re in2))))
(check-sat)
