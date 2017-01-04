
/*
 * xs :: [A]
 * tick :: A -> ()
 * xs foreach tick
 *
 * type delegate = (A -> (), [A])
 * run (tick, xs) = tick <$> xs
 *
 * type DelegateList = Map (A -> (), [A])
 */

struct Delegate {

};
